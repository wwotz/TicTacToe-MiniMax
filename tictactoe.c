#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <limits.h>

struct state_t {
	int cost;
	int depth;
	char turn;
	char position;
	char board[3][3];
};

struct state_stack_t {
	int size;
	int capacity;
	struct state_t *states;
};

#define MY_TURNP(turn) (turn == 0)
#define MIN_TURNP(turn) (turn == 0)
#define MAX_TURNP(turn) (turn == 1)

#define EMPTY_CELL  '.'
#define NOUGHT_CELL 'O'
#define CROSS_CELL  'X'
#define TURN_CELL(state) (state->turn ? NOUGHT_CELL : CROSS_CELL)

#define EMPTY_CELLP(cell)  (cell == EMPTY_CELL) 
#define NOUGHT_CELLP(cell) (cell == NOUGHT_CELL)
#define CROSS_CELLP(cell)  (cell == CROSS_CELL)

#define NOUGHT_COST 100
#define CROSS_COST -100
#define DRAW_COST   0
#define NO_COST     INT_MAX

static void
state_print(const struct state_t *state)
{
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			printf("%c", state->board[i][j]);
		}
		printf("\n");
	}
}

static struct state_t
state_empty(void)
{
	struct state_t state;
	memset(&state.turn, 0, sizeof(state.turn));
	memset(state.board, EMPTY_CELL, sizeof(state.board));
	state.position = -1;
	state.cost = 0;
	state.depth = 0;
	return state;
}

#define STATE_STACK_CAPACITY (8)

static struct state_stack_t
state_stack_empty(void)
{
	struct state_stack_t state_stack;
	memset(&state_stack, 0, sizeof(state_stack));
	state_stack.capacity = STATE_STACK_CAPACITY;
	state_stack.states = malloc(sizeof(*state_stack.states)
				    * state_stack.capacity);
	return state_stack;
}

static int
state_stack_resize(struct state_stack_t *state_stack)
{
	size_t new_capacity;
	struct state_t *new_states;
	new_capacity = state_stack->capacity * 2;
	new_states = realloc(state_stack->states,
			     sizeof(*new_states) * new_capacity);
	if (!new_states) {
		fprintf(stderr, "Error: Ran out of memory!");
		exit(EXIT_FAILURE);
	}

	state_stack->states = new_states;
	state_stack->capacity = new_capacity;
	return 0;
}

static int
state_stack_push(struct state_stack_t *state_stack, const struct state_t *state)
{
	if (state_stack->size == state_stack->capacity) {
		state_stack_resize(state_stack);
	}

	state_stack->states[state_stack->size++] = *state;
	return 0;
}

static struct state_t
state_stack_pop(struct state_stack_t *state_stack)
{
	if (state_stack->size == 0)
		return state_empty();
	return state_stack->states[--state_stack->size];
}

static void
state_stack_free(struct state_stack_t *state_stack)
{
	state_stack->size = 0;
	state_stack->capacity = 0;
	free(state_stack->states);
}

static int
state_cost(const struct state_t *state)
{
	int i,j;
	if (state->board[0][0] == state->board[1][1]
	    && state->board[1][1] == state->board[2][2]) {
		if (NOUGHT_CELLP(state->board[0][0])) return NOUGHT_COST - state->depth;
		if (CROSS_CELLP(state->board[0][0]))  return CROSS_COST + state->depth;
	}

	if (state->board[0][2] == state->board[1][1]
	    && state->board[1][1] == state->board[2][0]) {
		if (NOUGHT_CELLP(state->board[0][2])) return NOUGHT_COST - state->depth;
		if (CROSS_CELLP(state->board[0][2]))  return CROSS_COST + state->depth;
	}
	
	for (i = 0; i < 3; i++) {
		if (state->board[i][0] == state->board[i][1]
		    && state->board[i][1] == state->board[i][2]) {
			if (NOUGHT_CELLP(state->board[i][0])) return NOUGHT_COST - state->depth;
			if (CROSS_CELLP(state->board[i][0]))  return CROSS_COST + state->depth;
		}

		if (state->board[0][i] == state->board[1][i]
		    && state->board[1][i] == state->board[2][i]) {
			if (NOUGHT_CELLP(state->board[0][i])) return NOUGHT_COST - state->depth;
			if (CROSS_CELLP(state->board[0][i])) return CROSS_COST + state->depth;
		}
	}

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			if (EMPTY_CELLP(state->board[i][j])) {
				return NO_COST;
			}
		}
	}

	return DRAW_COST;
}

typedef enum winner_t {
	NONE = 0,
	NOUGHT_WINNER,
	CROSS_WINNER,
	DRAW,
} winner_t;

static winner_t
state_winner(const struct state_t *state)
{
	int i,j;
	if (state->board[0][0] == state->board[1][1]
	    && state->board[1][1] == state->board[2][2]) {
		if (NOUGHT_CELLP(state->board[0][0])) {
			return NOUGHT_WINNER;
		}
		
		if (CROSS_CELLP(state->board[0][0]))  {
			return CROSS_WINNER;
		}
	}

	if (state->board[0][2] == state->board[1][1]
	    && state->board[1][1] == state->board[2][0]) {
		if (NOUGHT_CELLP(state->board[0][2])) {
			return NOUGHT_WINNER;
		}
		
		if (CROSS_CELLP(state->board[0][2])) {
			return CROSS_WINNER;
		}
	}
	
	for (i = 0; i < 3; i++) {
		if (state->board[i][0] == state->board[i][1]
		    && state->board[i][1] == state->board[i][2]) {
			if (NOUGHT_CELLP(state->board[i][0])) {
				return NOUGHT_WINNER;
			}
			
			if (CROSS_CELLP(state->board[i][0]))  {
				return CROSS_WINNER;
			}
		}

		if (state->board[0][i] == state->board[1][i]
		    && state->board[1][i] == state->board[2][i]) {
			if (NOUGHT_CELLP(state->board[0][i])) {
				return NOUGHT_WINNER;
			}
			
			if (CROSS_CELLP(state->board[0][i])) {
				return CROSS_WINNER;
			}
		}
	}

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			if (EMPTY_CELLP(state->board[i][j])) {
				return NONE;
			}
		}
	}

	return DRAW;
}

static int
state_terminal(const struct state_t *state)
{
	int cost;
	cost = state_cost(state);
	if (cost == NO_COST) return 0;
	return 1;
}

static inline int
state_stack_emptyp(struct state_stack_t *state_stack)
{
	return state_stack->size == 0;
}

static void
state_stack_pushall(struct state_stack_t *state_stack, struct state_t *state)
{
	int i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			if (EMPTY_CELLP(state->board[i][j])) {
				int previous_position = state->position;
				state->board[i][j] = TURN_CELL(state);
				state->turn = 1 - state->turn;
				state->position = j + 3*i;
				state->depth += 1;
				state_stack_push(state_stack, state);
				state->board[i][j] = EMPTY_CELL;
				state->turn = 1 - state->turn;
				state->position = previous_position;
				state->depth -= 1;
			}
		}
	}
}

static struct state_t
state_max(struct state_t *state);

static struct state_t
state_min(struct state_t *state);

static struct state_t
state_minimax(struct state_t *state);

static struct state_t
state_max(struct state_t *state)
{
	int i, j;
	struct state_t current_state, best_state;
	struct state_stack_t state_stack = state_stack_empty();
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			if (EMPTY_CELLP(state->board[i][j])) {
				int previous_position = state->position;
				state->board[i][j] = TURN_CELL(state);
				state->turn = 1 - state->turn;
				state->position = previous_position == -1 ? j + 3 * i : previous_position;
				state->depth += 1;
				current_state = *state;
				current_state = state_minimax(&current_state);
				state_stack_push(&state_stack, &current_state);
				state->board[i][j] = EMPTY_CELL;
				state->turn = 1 - state->turn;
				state->position = previous_position;
				state->depth -= 1;
			}
		}
	}

	if (state_stack.size <= 0) return *state;

	best_state = state_stack.states[0];
	for (i = 1; i < state_stack.size; i++) {
		struct state_t *this_state = state_stack.states+i;
		if (best_state.cost < this_state->cost)
			best_state = *this_state;
	}

	memcpy(best_state.board, state->board, sizeof(best_state.board));
	j = best_state.position % 3;
	i = best_state.position / 3;
	best_state.board[i][j] = TURN_CELL(state);
	
	state_stack_free(&state_stack);
	return best_state;
}

static struct state_t
state_min(struct state_t *state)
{
	int i, j;
	struct state_t current_state, best_state;
	struct state_stack_t state_stack = state_stack_empty();
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			if (EMPTY_CELLP(state->board[i][j])) {
				int previous_position = state->position;
				state->board[i][j] = TURN_CELL(state);
				state->turn = 1 - state->turn;
				state->position = previous_position == -1 ? j + 3 * i : previous_position;
				state->depth += 1;
				current_state = *state;
				current_state = state_minimax(&current_state);
				state_stack_push(&state_stack, &current_state);
				state->board[i][j] = EMPTY_CELL;
				state->turn = 1 - state->turn;
				state->position = previous_position;
				state->depth -= 1;
			}
		}
	}

	if (state_stack.size <= 0) return *state;

	best_state = state_stack.states[0];
	for (i = 1; i < state_stack.size; i++) {
		struct state_t *this_state = state_stack.states+i;
		if (best_state.cost > this_state->cost)
			best_state = *this_state;
	}

	memcpy(best_state.board, state->board, sizeof(best_state.board));
	j = best_state.position % 3;
	i = best_state.position / 3;
	best_state.board[i][j] = TURN_CELL(state);
	
	state_stack_free(&state_stack);
	return best_state;
}

static struct state_t
state_solve(struct state_t *state)
{
	struct state_t new_state;
	new_state = state_minimax(state);
	new_state.position = -1;
	new_state.cost = 0;
	new_state.depth = 0;
	new_state.turn = 1 - state->turn;
	return new_state;
}

static struct state_t
state_minimax(struct state_t *state)
{
	int cost;
	cost = state_cost(state);
	if (cost != NO_COST) {
		state->cost = cost;
		return *state;
	}
	if (MAX_TURNP(state->turn)) {
		return state_max(state);
	}

	return state_min(state);
}

//static struct state_t
//state_minimax(struct state_t *state)
//{
//	int cost, current_cost, x, y;
//	struct state_stack_t state_stack;
//	struct state_t current_state, best_state;
//	state_stack = state_stack_empty();
//	state_stack_pushall(&state_stack, state);
//
//	cost = 0;
//	while (!state_stack_emptyp(&state_stack)) {
//		current_state = state_stack_pop(&state_stack);
//		if ((current_cost = state_cost(&current_state)) != NO_COST) {
//			if (current_cost <= cost && MAX_TURNP(current_state.turn)) {
//				cost = current_cost;
//				x = current_state.position % 3;
//				y = current_state.position / 3;
//				state->board[y][x] = TURN_CELL(state);
//				best_state = *state;
//				best_state.depth = current_state.depth;
//				best_state.turn = 1 - state->turn;
//				state->board[y][x] = EMPTY_CELL;
//			} else if (current_cost >= cost && MIN_TURNP(current_state.turn)) {
//				cost = current_cost;
//				x = current_state.position % 3;
//				y = current_state.position / 3;
//				state->board[y][x] = TURN_CELL(state);
//				best_state = *state;
//				best_state.depth = current_state.depth;
//				best_state.turn = 1 - state->turn;
//				state->board[y][x] = EMPTY_CELL;
//			}
//		}
//		state_stack_pushall(&state_stack, &current_state);
//	}
//
//	state_stack_free(&state_stack);
//	return best_state;
//}

static int
state_select(struct state_t *state, int cell)
{
	int x, y;

	if (cell < 0 || cell > 8) return 0;
	
	x = cell % 3;
	y = cell / 3;
	if (EMPTY_CELLP(state->board[y][x])) {
		state->board[y][x] = TURN_CELL(state);
		state->turn = 1 - state->turn;
		return 1;
	}
	
	return 0;
}

int
main(int argc, char **argv)
{
	int complete = 0, cell, cost;
	winner_t winner;
	struct state_t state = state_empty();
	printf("Welcome to TicTacToe!\n");
	state_print(&state);
	while (!complete) {
		if (MY_TURNP(state.turn)) {
			printf("Enter your cell [0-8]: ");
			scanf("%d", &cell);
			printf("Selection: %d\n", cell);
			if (!state_select(&state, cell)) {
				printf("Invalid option '%d'!\n", cell);
			}
		} else {
			state = state_solve(&state);
			printf("CPU finished.\n");
		}

		state_print(&state);
		winner = state_winner(&state);
		switch (winner) {
		case NOUGHT_WINNER:
			complete = 1;
			printf("You lose!\n");
			break;
		case CROSS_WINNER:
			complete = 1;
			printf("You Win!\n");
			break;
		case DRAW:
			complete = 1;
			printf("It's a draw!\n");
			break;
		default:
			break;
		}
	}
	return 0;
}

