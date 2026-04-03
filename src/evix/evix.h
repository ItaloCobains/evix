/*
 * evix.h -- API pública do event loop evix.
 *
 * Usa "opaque pointers": o usuário vê evix_loop_t* e evix_timer_t*
 * mas não sabe o que tem dentro das structs. Isso esconde a
 * implementação e permite mudar os internos sem quebrar quem usa.
 */

#ifndef EVIX_H
#define EVIX_H

#include <stdint.h>

/*
 * Forward declarations -- o "struct evix_loop" é definido em evix.c,
 * aqui só declaramos que ele existe. Quem inclui evix.h só pode
 * usar ponteiros pra ele, nunca acessar campos diretamente.
 */
typedef struct evix_loop evix_loop_t;
typedef struct evix_timer evix_timer_t;

/*
 * Tipo de callback que o event loop aceita.
 * É um ponteiro pra função que recebe void* (dados genéricos)
 * e não retorna nada. O void* permite passar qualquer coisa
 * (int*, struct*, etc.) -- o callback faz o cast de volta.
 */
typedef void (*evix_callback_fn)(void *data);

/* Cria um event loop alocado no heap. Retorna NULL se falhar. */
evix_loop_t *evix_loop_create(void);

/* Libera toda a memória do loop (callbacks, timers, o próprio loop). */
void         evix_loop_destroy(evix_loop_t *loop);

/*
 * Executa o event loop:
 * 1. Dispara todos os callbacks imediatos (registrados com add_callback).
 * 2. Entra em loop de timers: dorme até o próximo timer, dispara os
 *    expirados, repete até não ter mais timers.
 * Retorna 0 em sucesso.
 */
int          evix_loop_run(evix_loop_t *loop);

/*
 * Registra um callback imediato -- será chamado assim que evix_loop_run
 * iniciar, antes de processar qualquer timer. Ordem de execução é a
 * mesma ordem de registro (FIFO).
 */
void         evix_loop_add_callback(evix_loop_t *loop, evix_callback_fn callback, void *data);

/*
 * Cria e registra um timer no loop.
 *   delay_ms: tempo em milissegundos até o callback disparar.
 *   callback: função a ser chamada quando o timer expirar.
 *   data:     argumento passado ao callback.
 * O timer dispara uma única vez (one-shot) e é removido automaticamente.
 * Retorna ponteiro pro timer, ou NULL se falhar a alocação.
 */
evix_timer_t *evix_timer_create(evix_loop_t *loop, uint64_t delay_ms, evix_callback_fn callback, void *data);

/* Libera a memória de um timer (use só pra timers que você removeu manualmente). */
void          evix_timer_destroy(evix_timer_t *timer);

#endif
