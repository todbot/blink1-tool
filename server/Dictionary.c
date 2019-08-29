/*******************************************************************************
 * The MIT License (MIT)
 * 
 * Copyright (c) 2016 Jean-David Gadina - www-xs-labs.com
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "Dictionary.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

struct DictionaryItem
{
    struct DictionaryItem * next;
    const void            * key;
    const void            * value;
};

struct Dictionary
{
    struct DictionaryItem ** items;
    size_t                   count;
    size_t                   size;
    DictionaryCallbacks      callbacks;
};

static struct DictionaryItem * DictionaryGetItem( DictionaryRef d, const void * key );

#define DICTIONARY_HASH_MULTIPLIER  ( 31 )
#define DICTIONARY_MAX_LOAD_FACTOR  (  1 )
#define DICTIONARY_GROWTH_FACTOR    (  2 )

DictionaryRef DictionaryCreate( size_t size, DictionaryCallbacks * callbacks )
{
    struct Dictionary      * d;
    struct DictionaryItem ** t;
    
    if( size == 0 )
    {
        return NULL;
    }
    
    d = calloc( sizeof( struct Dictionary ), 1 );
    t = calloc( sizeof( struct DictionaryItem * ), size );
    
    if( d == NULL || t == NULL )
    {
        free( d );
        free( t );
        
        return NULL;
    }
    
    d->items = t;
    d->size  = size;
    
    if( callbacks )
    {
        d->callbacks = *( callbacks );
    }
    
    return d;
}

void DictionaryDelete( DictionaryRef d )
{
    if( d == NULL )
    {
        return;
    }
    
    DictionaryClear( d );
    
    free( d->items );
    free( d );
}

void DictionaryClear( DictionaryRef d )
{
    size_t                  i;
    struct DictionaryItem * item;
    struct DictionaryItem * del;
    
    if( d == NULL )
    {
        return;
    }
    
    if( d->items != NULL )
    {
        for( i = 0; i < d->size; i++ )
        {
            item          = d->items[ i ];
            d->items[ i ] = NULL;
            
            while( item )
            {
                del  = item;
                item = item->next;
                
                if( d->callbacks.kRelease )
                {
                    d->callbacks.kRelease( del->key );
                }
                
                if( d->callbacks.vRelease )
                {
                    d->callbacks.vRelease( del->value );
                }
                
                free( del );
                
                d->count--;
            }
        }
    }
}

void DictionaryInsert( DictionaryRef d, const void * key, const void * value )
{
    struct DictionaryItem * item;
    DictionaryHashCode      h;
    
    if( d == NULL || d->items == NULL || key == NULL || value == NULL )
    {
        return;
    }
    
    item = DictionaryGetItem( d, key );
    
    if( item )
    {
        if( d->callbacks.vRelease )
        {
            d->callbacks.vRelease( item->value );
        }
        
        if( d->callbacks.vRetain )
        {
            item->value = d->callbacks.vRetain( value );
        }
        else
        {
            item->value = value;
        }
        
        return;
    }
    
    item = calloc( sizeof( struct DictionaryItem ), 1 );
    
    if( item == NULL )
    {
        return;
    }
    
    if( d->callbacks.kRetain )
    {
        item->key = d->callbacks.kRetain( key );
    }
    else
    {
        item->key = key;
    }
    
    if( d->callbacks.vRetain )
    {
        item->value = d->callbacks.vRetain( value );
    }
    else
    {
        item->value = value;
    }
    
    if( d->callbacks.kHash )
    {
        h = d->callbacks.kHash( key );
    }
    else
    {
        h = ( uint64_t )key;
    }
    
    h = h % d->size;
    
    item->next    = d->items[ h ];
    d->items[ h ] = item;
    
    d->count++;
    
    if( d->count >= d->size * DICTIONARY_MAX_LOAD_FACTOR )
    {
        DictionaryResize( d, d->size * DICTIONARY_GROWTH_FACTOR );
    }
}

void DictionaryRemove( DictionaryRef d, const void * key )
{
    DictionaryHashCode      h;
    struct DictionaryItem * item;
    struct DictionaryItem * prev;
    
    if( d == NULL || d->items == NULL || key == NULL )
    {
        return;
    }
    
    if( d->callbacks.kHash )
    {
        h = d->callbacks.kHash( key );
    }
    else
    {
        h = ( uint64_t )key;
    }
    
    h = h % d->size;
    
    item = d->items[ h ];
    prev = NULL;
    
    while( item )
    {
        if( d->callbacks.kCompare == NULL && key != item->key )
        {
            prev = item;
            item = item->next;
            
            continue;
        }
        else if( key != item->key && d->callbacks.kCompare( key, item->key ) == false )
        {
            prev = item;
            item = item->next;
            
            continue;
        }
        
        if( prev == NULL )
        {
            d->items[ h ] = item->next;
        }
        else
        {
            prev->next = item->next;
        }
        
        if( d->callbacks.kRelease )
        {
            d->callbacks.kRelease( item->key );
        }
        
        if( d->callbacks.vRelease )
        {
            d->callbacks.vRelease( item->value );
        }
        
        free( item );
        
        d->count--;
        
        break;
    }
}

bool DictionaryKeyExists( DictionaryRef d, const void * key )
{
    return DictionaryGetItem( d, key ) != NULL;
}

const void * DictionaryGetValue( DictionaryRef d, const void * key )
{
    struct DictionaryItem * item;
    
    item = DictionaryGetItem( d, key );
    
    if( item )
    {
        return item->value;
    }
    
    return NULL;
}

size_t DictionaryGetSize( DictionaryRef d )
{
    if( d == NULL )
    {
        return 0;
    }
    
    return d->size;
}

size_t DictionaryGetCount( DictionaryRef d )
{
    if( d == NULL )
    {
        return 0;
    }
    
    return d->count;
}

void DictionaryShow( DictionaryRef d )
{
    if( d == NULL || d->items == NULL )
    {
        return;
    }
    
    fprintf( stderr, "<Dictionary 0x%lx> { size = %lu, count = %lu }\n", ( uintptr_t )d, d->size, d->count );
    
    if( d->count )
    {
        fprintf( stderr, "{\n" );
        
        {
            size_t                  i;
            struct DictionaryItem * item;
            
            for( i = 0; i < d->size; i++ )
            {
                item = d->items[ i ];
                
                while( item )
                {
                    fprintf( stderr, "    [ " );
                    
                    if( d->callbacks.kPrint )
                    {
                        d->callbacks.kPrint( stderr, item->key );
                    }
                    else
                    {
                        fprintf( stderr, "0x%lu", ( unsigned long )( item->key ) );
                    }
                    
                    fprintf( stderr, " ] => " );
                    
                    if( d->callbacks.vPrint )
                    {
                        d->callbacks.vPrint( stderr, item->value );
                    }
                    else
                    {
                        fprintf( stderr, "0x%lu", ( unsigned long )( item->value ) );
                    }
                    
                    fprintf( stderr, "\n" );
                    
                    item = item->next;
                }
            }
        }
        
        fprintf( stderr, "}\n" );
    }
}

void DictionarySwap( DictionaryRef d1, DictionaryRef d2 )
{
    struct Dictionary tmp;
    
    if( d1 == NULL || d2 == NULL )
    {
        return;
    }
    
    tmp     = *( d1 );
    *( d1 ) = *( d2 );
    *( d2 ) = tmp;
}

void DictionaryResize( DictionaryRef d, size_t size )
{
    size_t                  i;
    DictionaryRef           d2;
    struct DictionaryItem * item;
    DictionaryCallbacks     callbacks;
    
    if( d == NULL || d->items == NULL )
    {
        return;
    }
    
    d2 = DictionaryCreate( size, &( d->callbacks ) );
    
    if( d2 == NULL )
    {
        return;
    }
    
    callbacks             = d2->callbacks;
    d2->callbacks.kRetain = NULL;
    d2->callbacks.vRetain = NULL;
    
    for( i = 0; i < d->size; i++ )
    {
        item = d->items[ i ];
        
        while( item )
        {
            DictionaryInsert( d2, item->key, item->value );
            
            item = item->next;
        }
    }
    
    d2->callbacks         = callbacks;
    d->callbacks.kRelease = NULL;
    d->callbacks.vRelease = NULL;
    
    DictionarySwap( d, d2 );
    DictionaryDelete( d2 );
}

void DictionaryApplyFunction( DictionaryRef d, void ( * f )( const void *, const void * ) )
{
    size_t                  i;
    struct DictionaryItem * item;
    
    if( d == NULL || d->items == NULL || f == NULL )
    {
        return;
    }
    
    for( i = 0; i < d->size; i++ )
    {
        item = d->items[ i ];
        
        while( item )
        {
            f( item->key, item->value );
            
            item = item->next;
        }
    }
}

DictionaryCallbacks DictionaryStandardStringCallbacks( void )
{
    DictionaryCallbacks c;
    
    memset( &c, 0, sizeof( DictionaryCallbacks ) );
    
    c.kHash    = DictionaryHashStringCallback;
    c.kCompare = DictionaryCompareStringCallback;
    c.kRetain  = ( const void * ( * )( const void * ) )strdup;
    c.vRetain  = ( const void * ( * )( const void * ) )strdup;
    c.kRelease = ( void ( * )( const void * ) )free;
    c.vRelease = ( void ( * )( const void * ) )free;
    c.kPrint   = ( void ( * )( FILE *, const void * ) )fprintf;
    c.vPrint   = ( void ( * )( FILE *, const void * ) )fprintf;
    
    return c;
}

DictionaryHashCode DictionaryHashStringCallback( const void * key )
{
    DictionaryHashCode    h;
    unsigned char         c;
    const unsigned char * cp;
    
    if( key == NULL )
    {
        return 0;
    }
    
    cp = key;
    h  = 0;
    
    while( ( c = *( cp++ ) ) )
    {
        h = h * DICTIONARY_HASH_MULTIPLIER + c;
    }
    
    return h;
}

bool DictionaryCompareStringCallback( const void * k1, const void * k2 )
{
    if( k1 == NULL || k2 == NULL )
    {
        return false;
    }
    
    if( k1 == k2 )
    {
        return true;
    }
    
    return strcmp( k1, k2 ) == 0;
}

static struct DictionaryItem * DictionaryGetItem( DictionaryRef d, const void * key )
{
    DictionaryHashCode      h;
    struct DictionaryItem * item;
    
    if( d == NULL || d->items == NULL || key == NULL )
    {
        return false;
    }
    
    if( d->callbacks.kHash )
    {
        h = d->callbacks.kHash( key );
    }
    else
    {
        h = ( uint64_t )key;
    }
    
    h    = h % d->size;
    item = d->items[ h ];
    
    while( item )
    {
        if( key == item->key )
        {
            return item;
        }
        
        if( d->callbacks.kCompare && d->callbacks.kCompare( key, item->key ) )
        {
            return item;
        }
        
        item = item->next;
    }
    
    return NULL;
}
