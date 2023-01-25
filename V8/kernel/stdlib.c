/**
 * @file stdlib.c
 * @author Dejan Bogdanovic
 * 
 * Lightweight stdlib
 *
 * This file contains memory allocation and deallocation functions.
 * It should be replaced with C's library.
 */

#include <stdint.h>
#include "stdlib.h"

static mem_block_t * _mem_free = (mem_block_t *)HEAP_START;

void mem_init(void){
	_mem_free->_size = MAX_HEAP;
	_mem_free->_next = NULL;
}

void * malloc(uint32_t n){
	mem_block_t * _m_free = _mem_free, *_m_prev = NULL, *_m_next;
	void * _mem_block = NULL;
	n += sizeof(uint32_t);
	while(_m_free != NULL){
		if(_m_free->_size < n){
			_m_prev = _m_free;
			_m_free = _m_free->_next;
		}else{
			_mem_block = (void *)_m_free;
			if(_m_free->_size > n){
				_m_next = ((mem_block_t *)((void *)_m_free + n));
				_m_next->_size = _m_free->_size - n;
				_m_next->_next = _m_free->_next;			
			}else{
				_m_next = _m_free->_next;
			}
			if(_m_prev == NULL){
				_mem_free = _m_next;
			}else{
				_m_prev->_next = _m_next;
			}
			break;
		}
	}
	*((uint32_t *)_mem_block) = n;
	
	return _mem_block + sizeof(uint32_t);
}

void free(void * ptr){
	mem_block_t * _m_free = _mem_free, *_m_prev = NULL, *_m_mid;
	ptr -= sizeof(uint32_t);
	while(_m_free != NULL){
		if((void *)_m_free < ptr){
			_m_prev = _m_free;
			_m_free = _m_free->_next;
		}else{
			if(_m_prev == NULL){
				_m_prev = ((mem_block_t *)ptr);
				_m_prev->_size = *((uint32_t *)ptr);
				_m_prev->_next = _m_free;
				_mem_free = _m_prev;
			}else{
				_m_mid = ((mem_block_t *)ptr);
				_m_mid->_size = *((uint32_t *)ptr);
				_m_mid->_next = _m_free;
				_m_prev->_next = _m_mid;
			}
			break;
		}
	}
}

void memcp(char * to, char * from, uint32_t len){
	uint32_t i;
	for(i = 0; i < len; i++){
		*(to+i) = *(from+i);
	}
}

void memse(char * b, char c, uint32_t len){
	uint32_t i;
	for(i = 0; i < len; i++){
		*(b+i) = c;
	}
}

uint32_t memcm(char * a, char * b, uint32_t len){
	uint32_t i;
	for(i = 0; i < len; i++){
		if(*(a+i) != *(b+i)){
			return 0;
		}
	}
	
	return 1;
}

