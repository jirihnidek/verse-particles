/*
 * $Id$
 *
 * ***** BEGIN BSD LICENSE BLOCK *****
 *
 * Copyright (c) 2009-2011, Jiri Hnidek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ***** END BSD LICENSE BLOCK *****
 *
 * Authors: Jiri Hnidek <jiri.hnidek@tul.cz>
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <verse.h>

#include "lu_table.h"

void lu_table_free(struct LookUp_Table *lt)
{
	if(lt->lu_items != NULL) {
		free(lt->lu_items);
		lt->lu_items = NULL;
	}
}

struct LookUp_Table *lu_table_create(uint16 size)
{
	struct LookUp_Table *lt;

	lt = (struct LookUp_Table*)malloc(sizeof(struct LookUp_Table));

	if(lt != NULL) {
		lt->lu_items = (struct LookUp_Item*)calloc(size, sizeof(struct LookUp_Item));

		if(lt->lu_items != NULL) {
			int i;
			for(i=0; i<size; i++) {
				lt->lu_items[i].id = -1;
			}
			lt->size = size;
			lt->count = 0;
		} else {
			free(lt);
			lt = NULL;
		}
	}

	return lt;
}

int lu_add_item(struct LookUp_Table *lu, uint32 id, void *item)
{
	if(lu->count < lu->size) {
		struct LookUp_Item *lu_item = &lu->lu_items[id % lu->size];

		if(lu_item->id != (uint16)-1 && lu_item->item == NULL) {
			lu_item->id = id;
			lu_item->item = item;
			lu->count++;
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

void lu_rem_item(struct LookUp_Table *lu, uint32 id)
{
	struct LookUp_Item *lu_item = &lu->lu_items[id % lu->size];

	lu_item->id = -1;
	lu_item->item = NULL;

	lu->count--;
}

void *lu_find(struct LookUp_Table *lu, uint32 id)
{
	struct LookUp_Item *lu_item = &lu->lu_items[id % lu->size];

	if(lu_item->id == id && lu_item->item != NULL) {
		return lu_item->item;
	}

	return NULL;

}
