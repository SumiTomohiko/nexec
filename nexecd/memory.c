#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_SIZE  8192

struct arena {
	struct arena	*next;
	size_t		used_size;
	void		*ptr;
};

struct storage {
	struct arena	*arena;
};

static struct storage storage;

static struct arena *
allocate_arena()
{
	struct arena *arena;
	void *ptr;

	arena = (struct arena *)malloc(sizeof(struct arena));
	assert(arena != NULL);
	ptr = malloc(ARENA_SIZE);
	assert(ptr != NULL);
	memset(ptr, 0xab, ARENA_SIZE);
	arena->next = NULL;
	arena->used_size = 0;
	arena->ptr = ptr;
	return (arena);
}

void
memory_initialize()
{

	storage.arena = allocate_arena();
}

void
memory_dispose()
{
	struct arena *arena, *next;

	arena = storage.arena;
	while (arena != NULL) {
		next = arena->next;
		free(arena->ptr);
		free(arena);
		arena = next;
	}
}

void *
memory_allocate(size_t size)
{
	struct arena *arena;
	size_t aligned_size, unused_size, used_size;
	void *ptr;

	if (size == 0)
		return (NULL);
	aligned_size = ((size - 1) / sizeof(size_t) + 1) * sizeof(size_t);
	used_size = storage.arena->used_size;
	unused_size = ARENA_SIZE - used_size;
	if (unused_size < aligned_size) {
		arena = allocate_arena();
		arena->next = storage.arena;
		storage.arena = arena;
		return (memory_allocate(size));
	}
	ptr = (void *)((uintptr_t)storage.arena->ptr + used_size);
	storage.arena->used_size += aligned_size;
	return (ptr);
}
