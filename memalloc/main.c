#include <stdio.h>
#include <windows.h>
#include <stdint.h>

// Block header structure
typedef struct Block {
    size_t size;
    int free;
    struct Block* next;
    struct Block* prev;
} Block;

#define BLOCK_SIZE sizeof(Block)

Block* free_list = NULL;

void* malloc(size_t size);
void* realloc(void* ptr, size_t new_size);
void free(void* ptr);
Block* find_free_block(size_t size);
Block* request_space(size_t size);
void split_block(Block* block, size_t size);

void* malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }

    Block* block;
    if (free_list) {
        block = find_free_block(size);
        if (block) {
            block->free = 0;
            if (block->size > size + BLOCK_SIZE) {
                split_block(block, size);
            }
        } else {
            block = request_space(size);
            if (!block) {
                return NULL;
            }
        }
    } else {
        block = request_space(size);
        if (!block) {
            return NULL;
        }
    }

    return (block + 1);
}

void free(void* ptr) {
    if (!ptr) return;

    Block* block = (Block*)ptr - 1;
    block->free = 1;

    if (block->next && block->next->free) {
        block->size += BLOCK_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->free) {
        block->prev->size += BLOCK_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    } else {
        block->next = free_list;
        block->prev = NULL;
        if (free_list) {
            free_list->prev = block;
        }
        free_list = block;
    }
}

Block* find_free_block(size_t size) {
    Block* current = free_list;
    while (current && !(current->free && current->size >= size)) {
        current = current->next;
    }
    return current;
}

Block* request_space(size_t size) {
    size_t total_size = size + BLOCK_SIZE;
    void* request = VirtualAlloc(NULL, total_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (request == NULL) {
        return NULL; // VirtualAlloc failed
    }

    Block* block = (Block*)request;
    block->size = size;
    block->free = 0;
    block->next = NULL;
    block->prev = NULL;

    return block;
}


void* realloc(void* ptr, size_t new_size) {
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return malloc(new_size);
    }

    Block* old_block = (Block*)ptr - 1;
    size_t old_size = old_block->size;
    void* new_ptr = malloc(new_size);
    if (!new_ptr) {
        return NULL; // Allocation failed
    }


    size_t min_size = old_size < new_size ? old_size : new_size;
    char* old_data = (char*)ptr;
    char* new_data = (char*)new_ptr;

    for (size_t i = 0; i < min_size; i++) {
        new_data[i] = old_data[i];
    }

    free(ptr);

    return new_ptr;
}

void split_block(Block* block, size_t size) {
    Block* new_block = (Block*)((char*)block + BLOCK_SIZE + size);
    new_block->size = block->size - size - BLOCK_SIZE;
    new_block->free = 1;
    new_block->next = block->next;
    new_block->prev = block;
    if (block->next) {
        block->next->prev = new_block;
    }
    block->size = size;
    block->next = new_block;
}

// Test the implementation
int main() {
    void* ptr1 = malloc(15);
    printf("Allocated 15 bytes at %p\n", ptr1);

    void* ptr2 = malloc(8);
    printf("Allocated 8 bytes at %p\n", ptr2);

    free(ptr1);
    printf("Freed 15 bytes\n");

    void* ptr3 = realloc(ptr2, 11);
    printf("Reallocated to 11 bytes at %p\n", ptr3);

    void* ptr4 = malloc(16);
    printf("Allocated 16 bytes at %p\n", ptr4);

    free(ptr3);
    printf("Freed 11 bytes\n");

    return 0;
}
