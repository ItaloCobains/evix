#include "evix.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/*
 * Retorna o tempo atual em milissegundos usando CLOCK_MONOTONIC.
 *
 * CLOCK_MONOTONIC é um relógio que só avança -- não é afetado se
 * alguém ajusta a hora do sistema (diferente de CLOCK_REALTIME).
 * Isso é essencial pra medir intervalos: se você agenda um timer
 * pra daqui a 100ms, não quer que ele dispare errado porque
 * alguém mudou o relógio do sistema.
 *
 * clock_gettime preenche um struct timespec com:
 *   tv_sec  = segundos inteiros.
 *   tv_nsec = nanossegundos (parte fracionária).
 * A gente converte tudo pra milissegundos pra simplificar.
 */
static uint64_t now_ms(void)
{
    struct timespec time_spec;
    clock_gettime(CLOCK_MONOTONIC, &time_spec);
    return ((uint64_t)time_spec.tv_sec * SEC_TO_MS)
         + ((uint64_t)time_spec.tv_nsec / MS_TO_NS);
}

/*
 * Dorme por um número de milissegundos.
 *
 * nanosleep suspende o processo (o SO tira ele da fila de execução).
 * Diferente de um busy wait (while loop checando a hora), isso não
 * consome CPU enquanto espera.
 *
 * nanosleep recebe um struct timespec, então precisamos converter
 * milissegundos de volta pra segundos + nanossegundos.
 */
static void sleep_ms(uint64_t milliseconds)
{
    struct timespec time_spec;
    time_spec.tv_sec = (time_t)(milliseconds / SEC_TO_MS);
    time_spec.tv_nsec = (long)((milliseconds % SEC_TO_MS) * MS_TO_NS);
    nanosleep(&time_spec, NULL);
}

/* ================================================================
 * ESTRUTURAS INTERNAS (não expostas no header)
 * ================================================================ */

/*
 * Nó de uma linked list de callbacks imediatos.
 *
 * Linked list porque:
 * - Inserção em O(1) se mantivéssemos um ponteiro pro tail
 *   (hoje é O(n), melhoria possível).
 * - Não precisa saber o tamanho antecipadamente (vs array).
 * - Cada nó é alocado independentemente com calloc.
 *
 * callback: ponteiro pra função a ser chamada.
 * data:     argumento genérico passado ao callback (void*).
 * next:     próximo nó da lista, ou NULL se for o último.
 */
typedef struct evix_cb_node {
    evix_callback_fn callback;
    void *data;
    struct evix_cb_node *next;
} evix_cb_node_t;

/*
 * Timer: um callback que só deve executar após um certo tempo.
 *
 * expire_at: timestamp absoluto em ms (CLOCK_MONOTONIC) de quando
 *            o timer deve disparar. Calculado como now_ms() + delay_ms
 *            no momento do registro. Usamos tempo absoluto (não relativo)
 *            pra evitar acúmulo de erro se o loop demorar pra iterar.
 * callback:  função chamada quando o timer expira.
 * data:      argumento passado ao callback.
 * next:      próximo timer na lista, ou NULL.
 */
struct evix_timer {
    uint64_t expire_at;
    evix_callback_fn callback;
    void *data;
    struct evix_timer *next;
};

/*
 * O event loop em si.
 *
 * running: flag reservada pra controle futuro (ex: evix_loop_stop).
 * head:    início da lista de callbacks imediatos.
 * timers:  início da lista de timers pendentes.
 *
 * Quando head e timers são ambos NULL, o loop não tem trabalho
 * e evix_loop_run retorna imediatamente.
 */
struct evix_loop {
    int running;
    evix_cb_node_t *head;
    evix_timer_t *timers;
    evix_backend_t *backend;
};

/* ================================================================
 * LIFECYCLE: criar e destruir o loop.
 * ================================================================ */

/*
 * calloc(1, sizeof) aloca E zera a memória.
 * Isso garante que head=NULL, timers=NULL, running=0.
 * Se usasse malloc, os campos teriam lixo.
 */
evix_loop_t *evix_loop_create(evix_backend_t *backend)
{
    evix_loop_t *loop = calloc(1, sizeof(evix_loop_t));
    
    if (!loop) {
        return NULL;
    }

    loop->backend = backend;

    if (backend && backend->init) {
        backend->init(loop);
    }

    return loop;
}

/*
 * Libera toda memória associada ao loop:
 * 1. Percorre a lista de callbacks, liberando cada nó.
 * 2. Percorre a lista de timers, liberando cada nó.
 * 3. Libera o próprio loop.
 *
 * O padrão "salvar next antes de free" é necessário porque
 * depois do free(node), acessar node->next é undefined behavior
 * (a memória já foi devolvida ao SO).
 */
void evix_loop_destroy(evix_loop_t *loop)
{
    evix_cb_node_t *node = loop->head;
    while (node) {
        evix_cb_node_t *next = node->next;
        free(node);
        node = next;
    }

    evix_timer_t *timer = loop->timers;
    while (timer) {
        evix_timer_t *next = timer->next;
        free(timer);
        timer = next;
    }

    if (loop->backend && loop->backend->destroy) {
        loop->backend->destroy(loop);
    }

    free(loop);
}

/* ================================================================
 * CORE: o event loop.
 * ================================================================ */

/*
 * Executa o event loop. Duas fases:
 *
 * FASE 1 -- Callbacks imediatos.
 *   Percorre a linked list e chama cada callback na ordem de registro.
 *   Simples: só itera e chama.
 *
 * FASE 2 -- Timers.
 *   Loop externo: roda enquanto existirem timers pendentes.
 *   Cada iteração:
 *
 *   a) Achar o timer mais próximo (menor expire_at).
 *      Percorre toda a lista -- O(n). Num event loop real usaria
 *      uma min-heap pra O(1), mas pra poucos timers não importa.
 *
 *   b) Dormir até esse timer.
 *      Calcula quanto falta (earliest - now) e chama sleep_ms.
 *      O processo dorme sem consumir CPU. Isso é o que diferencia
 *      de busy wait.
 *
 *   c) Disparar timers expirados.
 *      Percorre a lista e chama o callback de cada timer cujo
 *      expire_at <= agora. Remove o timer da lista e libera memória.
 *
 *      A remoção usa o padrão "ponteiro pra ponteiro" (**prev):
 *      - prev aponta pro campo "next" do nó anterior (ou pro head).
 *      - pra remover: *prev = timer->next (o anterior agora
 *        aponta pro próximo, pulando o nó removido).
 *      - se não remove: prev avança pra &timer->next.
 *      Isso evita tratar o caso "remover o primeiro nó" separadamente.
 */
int evix_loop_run(evix_loop_t *loop)
{
    /* FASE 1: callbacks imediatos. */
    evix_cb_node_t *node = loop->head;
    while (node) {
        node->callback(node->data);
        node = node->next;
    }

    /* FASE 2: timers. */
    while (loop->timers) {
        /* (a) Achar o timer mais próximo. */
        uint64_t earliest = loop->timers->expire_at;
        evix_timer_t *timerNode = loop->timers->next;
        while (timerNode) {
            if (timerNode->expire_at < earliest) {
                earliest = timerNode->expire_at;
            }
            timerNode = timerNode->next;
        }

        /* (b) Dormir até o timer mais próximo. */
        uint64_t current = now_ms();
        if (earliest > current) {
            sleep_ms(earliest - current);
        }

        /* (c) Disparar todos os timers expirados. */
        current = now_ms();
        evix_timer_t **prev = &loop->timers;
        evix_timer_t *timer = loop->timers;
        while (timer) {
            if (current >= timer->expire_at) {
                timer->callback(timer->data);
                *prev = timer->next;
                evix_timer_t *dead = timer;
                timer = timer->next;
                free(dead);
            } else {
                prev = &timer->next;
                timer = timer->next;
            }
        }
    }

    return 0;
}

/* ================================================================
 * CALLBACKS IMEDIATOS.
 * ================================================================ */

/*
 * Insere um callback no final da lista (FIFO).
 *
 * Se a lista está vazia (head == NULL), o novo nó vira o head.
 * Senão, percorre até o último nó e encadeia.
 *
 * calloc zera o nó, então node->next já é NULL.
 */
void evix_loop_add_callback(evix_loop_t *loop, evix_callback_fn callback, void *data)
{
    evix_cb_node_t *node = calloc(1, sizeof(evix_cb_node_t));

    if (!node) {
        return;
    }

    node->callback = callback;
    node->data = data;

    if (!loop->head) {
        loop->head = node;
    } else {
        evix_cb_node_t *tail = loop->head;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = node;
    }
}

/* ================================================================
 * TIMERS.
 * ================================================================ */

/*
 * Cria um timer e registra no loop.
 *
 * expire_at é calculado como tempo absoluto: now_ms() + delay_ms.
 * Se delay_ms == 0, o timer expira imediatamente na próxima iteração.
 *
 * Inserção é feita no INÍCIO da lista (prepend) -- O(1).
 * A ordem não importa porque o loop sempre procura o menor expire_at.
 *
 * Retorna o ponteiro pro timer (pode ser usado pra cancelamento futuro).
 */
evix_timer_t *evix_timer_create(evix_loop_t *loop, uint64_t delay_ms, evix_callback_fn callback, void *data)
{
    evix_timer_t *timer = calloc(1, sizeof(evix_timer_t));

    if (!timer) {
        return NULL;
    }

    timer->expire_at = now_ms() + delay_ms;
    timer->callback = callback;
    timer->data = data;

    timer->next = loop->timers;
    loop->timers = timer;

    return timer;
}

/*
 * Libera a memória de um timer.
 * Atenção: não remove da lista do loop. Use só pra timers que já
 * foram removidos (ex: já dispararam) ou que você removeu manualmente.
 */
void evix_timer_destroy(evix_timer_t *timer)
{
    free(timer);
}
