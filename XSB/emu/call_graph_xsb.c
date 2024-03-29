/* File:      call_graph_xsb.c
** Author(s): Diptikalyan Saha, C. R. Ramakrishnan
** Contact:   xsb-contact@cs.sunysb.edu
** 
** Copyright (C) The Research Foundation of SUNY, 1986, 1993-1998
** Copyright (C) ECRC, Germany, 1990
** 
** XSB is free software; you can redistribute it and/or modify it under the
** terms of the GNU Library General Public License as published by the Free
** Software Foundation; either version 2 of the License, or (at your option)
** any later version.
** 
** XSB is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License for
** more details.
** 
** You should have received a copy of the GNU Library General Public License
** along with XSB; if not, write to the Free Software Foundation,
** Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** 
** 
*/
 

#include "xsb_config.h"
#include "xsb_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Special debug includes */
/* Remove the includes that are unncessary !!*/
#include "hashtable.h"
#include "hashtable_itr.h"
#include "debugs/debug_tries.h"
#include "auxlry.h"
#include "context.h"   
#include "cell_xsb.h"   
#include "inst_xsb.h"
#include "psc_xsb.h"
#include "heap_xsb.h"
#include "flags_xsb.h"
#include "deref.h"
#include "memory_xsb.h" 
#include "register.h"
#include "binding.h"
#include "trie_internals.h"
#include "tab_structs.h"
#include "choice.h"
#include "subp.h"
#include "cinterf.h"
#include "error_xsb.h"
#include "tr_utils.h"
#include "hash_xsb.h" 
#include "call_xsb_i.h"
#include "tst_aux.h"
#include "tst_utils.h"
#include "token_xsb.h"
#include "call_graph_xsb.h"
#include "thread_xsb.h"
#include "loader_xsb.h" /* for ZOOM_FACTOR, used in stack expansion */
#include "tries.h"
#include "tr_utils.h"
#include "debug_xsb.h"

#define INCR_DEBUG2

/* 
Terminology: outedge -- pointer to calls that depend on call
             inedge  -- pointer to calls on which call depends
*/

/********************** STATISTICS *****************************/

//int cellarridx_gl;
//int maximum_dl_gl=0;
//callnodeptr callq[20000000];
//int callqptr=0;
//int no_add_call_edge_gl=0;
//int saved_call_gl=0,
//int factcount_gl=0;

// affected_gl drives create_call_list and updtes
calllistptr affected_gl=NULL;
calllistptr lazy_affected= NULL;

// derives create_changed_call_list which reports those calls changed in last update
calllistptr changed_gl=NULL;

// used to compare new to previous tables.
callnodeptr old_call_gl=NULL;
// is this needed?  Can we use the root ptr from SF?
BTNptr old_answer_table_gl=NULL;

call2listptr marked_list_gl=NULL; /* required for abolishing incremental calls */ 
calllistptr assumption_list_gl=NULL;  /* required for abolishing incremental calls */ 

// current number of incremental subgoals / edges
int call_node_count_gl=0,call_edge_count_gl=0;
// total number of incremental subgoals in session.
int  call_count_gl=0;  
// not used much -- for statistics
int unchanged_call_gl=0;

// Maximum arity 
static Cell cell_array1[500];

Structure_Manager smCallNode  =  SM_InitDecl(CALL_NODE,CALLNODE_PER_BLOCK,"CallNode");
Structure_Manager smCallList  =  SM_InitDecl(CALLLIST,CALLLIST_PER_BLOCK,"CallList");
Structure_Manager smCall2List =  SM_InitDecl(CALL2LIST,CALL2LIST_PER_BLOCK,"Call2List");
Structure_Manager smKey	      =  SM_InitDecl(KEY,KEY_PER_BLOCK,"HashKey");
Structure_Manager smOutEdge   =  SM_InitDecl(OUTEDGE,OUTEDGE_PER_BLOCK,"Outedge");


DEFINE_HASHTABLE_INSERT(insert_some, KEY, CALL_NODE);
DEFINE_HASHTABLE_SEARCH(search_some, KEY, callnodeptr);
DEFINE_HASHTABLE_REMOVE(remove_some, KEY, callnodeptr);
DEFINE_HASHTABLE_ITERATOR_SEARCH(search_itr_some, KEY);


/*****************************************************************************/
static unsigned int hashid(void *ky)
{
    return (long int)ky;
}

static unsigned int hashfromkey(void *ky)
{
    KEY *k = (KEY *)ky;
    return (int)(k->goal);
}

static int equalkeys(void *k1, void *k2)
{
    return (0 == memcmp(k1,k2,sizeof(KEY)));
}



/*****************************************************************************/

void printcall(callnodeptr c){
  //  printf("%d",c->id);
  if(IsNonNULL(c->goal))
    sfPrintGoal(stdout,c->goal,NO);
  else
    printf("fact");
  return;
}

/******************** GENERATION OF CALLED_BY GRAPH ********************/

/* Creates a call node */
callnodeptr makecallnode(VariantSF sf){
  
  callnodeptr cn;

  SM_AllocateStruct(smCallNode,cn);
  cn->deleted = 0;
  cn->changed = 0;
  cn->no_of_answers = 0;
  cn->falsecount = 0;
  cn->recomputable = COMPUTE_DEPENDENCIES_FIRST;
  cn->prev_call = NULL;
  cn->aln = NULL;
  cn->inedges = NULL;
  cn->goal = sf;
  cn->outedges=NULL;
  cn->id=call_count_gl++; 
  cn->outcount=0;

  //    printcall(cn);printf("\n");

  call_node_count_gl++;
  //  printf("makecallnode %p sf %p\n",cn,sf);
  return cn;
}


void deleteinedges(callnodeptr callnode){
  calllistptr tmpin,in;
  
  KEY *ownkey;
  struct hashtable* hasht;
  SM_AllocateStruct(smKey, ownkey);
  ownkey->goal=callnode->id;	
	
  in = callnode->inedges;
  
  while(IsNonNULL(in)){
    tmpin = in->next;
    hasht = in->inedge_node->hasht;
    //    printf("remove some callnode %x / ownkey %d\n",callnode,ownkey);
    if (remove_some(hasht,ownkey) == NULL) {
      xsb_abort("BUG: key not found for removal\n");
    }
    call_edge_count_gl--;
    SM_DeallocateStruct(smCallList, in);      
    in = tmpin;
  }
  SM_DeallocateStruct(smKey, ownkey);      
  return;
}

/* used for abolishes -- its known that outcount is 0 */
void deletecallnode(callnodeptr callnode){

  call_node_count_gl--;
   
  if(callnode->outcount==0){
    hashtable1_destroy(callnode->outedges->hasht,0);
    SM_DeallocateStruct(smOutEdge, callnode->outedges);      
    SM_DeallocateStruct(smCallNode, callnode);        
  }else
    xsb_abort("outcount is nonzero\n");
  
  return;
}


void deallocate_previous_call(callnodeptr callnode){
  
  calllistptr tmpin,in;
  
  KEY *ownkey;
  /*  callnodeptr  inedge_node; */
  struct hashtable* hasht;
  SM_AllocateStruct(smKey, ownkey);
  ownkey->goal=callnode->id;	
	
  in = callnode->inedges;
  call_node_count_gl--;
  
  while(IsNonNULL(in)){
    tmpin = in->next;
    hasht = in->inedge_node->hasht;
    if (remove_some(hasht,ownkey) == NULL) {
      /*
      prevnode=in->prevnode->callnode;
      if(IsNonNULL(prevnode->goal)){
	sfPrintGoal(stdout,(VariantSF)prevnode->goal,NO); printf("(%d)",prevnode->id);
      }
      if(IsNonNULL(callnode->goal)){
	sfPrintGoal(stdout,(VariantSF)callnode->goal,NO); printf("(%d)",callnode->id);
      }
      */
      xsb_abort("BUG: key not found for removal\n");
    }
    in->inedge_node->callnode->outcount--;
    call_edge_count_gl--;
    SM_DeallocateStruct(smCallList, in);      
    in = tmpin;
  }
  
  SM_DeallocateStruct(smCallNode, callnode);      
  SM_DeallocateStruct(smKey, ownkey);      
}

void initoutedges(callnodeptr cn){
  outedgeptr out;

#ifdef INCR_DEBUG
	printf("Initoutedges %d\n",cn->id);
#endif

  SM_AllocateStruct(smOutEdge,out);
  cn->outedges = out;
  out->callnode = cn; 	  
  out->hasht =create_hashtable1(HASH_TABLE_SIZE, hashfromkey, equalkeys);
  return;
}


/*
propagate_no_change(c)
	for c->c'
	  if(c'.deleted=false)
	     c'->falsecount>0
	       c'->falsecount--
	       if(c'->falsecount==0)
		 propagate_no_change(c')		

When invalidation is done a parameter 'falsecount' is maintained with
each call which signifies that these many predecessor calls have been
affected. So if a call A has two pred node B and C and both of them
are affected then A's falsecount is 2. Now when B is reevaluated and
turns out it has not been changed (its old and new answer table is the
same) completion of B calls propagate_no_change(B) which reduces the
falsecount of A by 1. If for example turns out that C was also not
changed the falsecount of A is going to be reduced to 0. Now when call
A is executed it's just going to do answer clause resolution.
*/
void propagate_no_change(callnodeptr c){
  callnodeptr cn;
  struct hashtable *h;	
  struct hashtable_itr *itr;
  if(IsNonNULL(c)){
    h=c->outedges->hasht;
    itr = hashtable1_iterator(h);
    if (hashtable1_count(h) > 0){
      do {
	cn= hashtable1_iterator_value(itr);
	if(cn->falsecount>0){ /* this check is required for the new dependencies that can arise bcoz of the re-evaluation */
	  cn->falsecount--;
	  if(cn->falsecount==0){
	    cn->deleted = 0;
	    propagate_no_change(cn);
	  }
	}
      } while (hashtable1_iterator_advance(itr));
    }
  }		
}

/* Enter a call to calllist */
static void inline add_callnode_sub(calllistptr *list, callnodeptr item){
  calllistptr  temp;
  SM_AllocateStruct(smCallList,temp);
  temp->item=item;
  temp->next=*list;
  *list=temp;  
  //  printf("added list %p @list %p\n",list,*list);
}

static void inline add_calledge(calllistptr *list, outedgeptr item){
  calllistptr  temp;
  SM_AllocateStruct(smCallList,temp);
  temp->inedge_node=item;
  temp->next=*list;
  *list=temp;  
}

static void inline ecall3(calllistptr *list, call2listptr item){
  calllistptr  temp;
  SM_AllocateStruct(smCallList,temp);
  temp->item2=item;
  temp->next=*list;
  *list=temp;  
}

void addcalledge(callnodeptr fromcn, callnodeptr tocn){
  
  KEY *k1;
  SM_AllocateStruct(smKey, k1);
  k1->goal = tocn->id;
  
  // #ifdef INCR_DEBUG
  //  printf("%d-->%d",fromcn->id,tocn->id);	
  // #endif
  
  
  if (NULL == search_some(fromcn->outedges->hasht,k1)) {
    insert_some(fromcn->outedges->hasht,k1,tocn);

#ifdef INCR_DEBUG	
    printf("Inedges of %d = ",tocn->id);
    temp=tocn->inedges;
    while(temp!=NULL){
      printf("\t%d",temp->inedge_node->callnode->id);
      temp=temp->next;
    }
    printf("\n");
#endif
    
    add_calledge(&(tocn->inedges),fromcn->outedges);      
    call_edge_count_gl++;
    fromcn->outcount++;
    
#ifdef INCR_DEBUG		
    if(IsNonNULL(fromcn->goal)){
      sfPrintGoal(stdout,(VariantSF)fromcn->goal,NO);printf("(%d)",fromcn->id);
    }else
      printf("(%d)",fromcn->id);
    
    if(IsNonNULL(tocn->goal)){
      printf("-->");	
      sfPrintGoal(stdout,(VariantSF)tocn->goal,NO);printf("(%d)",tocn->id);
    }
    printf("\n");	
#endif
    
    
  }

#ifdef INCR_DEBUG
	printf("Inedges of %d = ",tocn->id);
	temp=tocn->inedges;
	while(temp!=NULL){
		printf("\t%d",temp->inedge_node->callnode->id);
		temp=temp->next;
	}
	printf("\n---------------------------------\n");
#endif
}

#define EMPTY NULL

//calllistptr eneetq(){ 
calllistptr empty_calllist(){ 
  
  calllistptr  temp;
  SM_AllocateStruct(smCallList,temp);
  temp->item = (callnodeptr)EMPTY;
  temp->next = NULL;

  return temp;
}


void add_callnode(calllistptr *cl,callnodeptr c){
  add_callnode_sub(cl,c);
}

callnodeptr delete_calllist_elt(calllistptr *cl){   
  
  calllistptr tmp;
  callnodeptr c;

  c = (*cl)->item;
  tmp = *cl;
  *cl = (*cl)->next;
  //  printf("calllist %p item %p next %p\n",tmp,c,*cl);
  SM_DeallocateStruct(smCallList, tmp);      
  
  return c;  
}

/* Used to deallocate dfs-created call lists when encountering
   visitors or incomlete tables. */
void deallocate_call_list(calllistptr cl)  {
    callnodeptr tmp_call;

    //    fprintf(stddbg," in deallocate call list \n");
    while ((tmp_call = delete_calllist_elt(&cl)) != EMPTY){
      ;
    }
    //    SM_DeallocateStruct(smCallList, cl);      
  }

void dfs_outedges_check_non_completed(CTXTdeclc callnodeptr call1) {
  char bufferb[MAXTERMBUFSIZE]; 

  if(IsNonNULL(call1->goal) && !subg_is_completed((VariantSF)call1->goal)){
  deallocate_call_list(affected_gl);
  sprint_subgoal(CTXTc forest_log_buffer_1,0,(VariantSF)call1->goal);     
  sprintf(bufferb,"Incremental tabling is trying to invalidate an incomplete table \n %s\n",
	  forest_log_buffer_1->fl_buffer);
  xsb_new_table_error(CTXTc "incremental_tabling",bufferb,
		      get_name(TIF_PSC(subg_tif_ptr(call1->goal))),
		      get_arity(TIF_PSC(subg_tif_ptr(call1->goal))));
  }
}

typedef struct incr_callgraph_dfs_frame {
  hashtable_itr_ptr itr;
  callnodeptr cn;
} IncrCallgraphDFSFrame;

#define push_dfs_frame(CN,ITR)	{					\
    /*    printf("pushing cn %p   ",CN);				\
    if (CN -> goal) { printf("*** ");print_subgoal(stddbg,CN->goal);}  printf("\n"); */\
    ctr ++;								\
    if (++incr_callgraph_dfs_top == incr_callgraph_dfs_size) {		\
      printf("reallocing to %d\n",incr_callgraph_dfs_size*2);		\
      mem_realloc(incr_callgraph_dfs, incr_callgraph_dfs_size*sizeof(IncrCallgraphDFSFrame), \
		  2*incr_callgraph_dfs_size*sizeof(IncrCallgraphDFSFrame), TABLE_SPACE); \
      incr_callgraph_dfs_size =     2*incr_callgraph_dfs_size;		\
    }									\
    incr_callgraph_dfs[incr_callgraph_dfs_top].itr = ITR;		\
    incr_callgraph_dfs[incr_callgraph_dfs_top].cn = CN;		\
  }

#define pop_dfs_frame {incr_callgraph_dfs_top--;}

void dfs_outedges(CTXTdeclc callnodeptr call1){
  callnodeptr cn;
  struct hashtable *h;	
  struct hashtable_itr *itr;
  int ctr = 0;

  int incr_callgraph_dfs_top = -1;
  int incr_callgraph_dfs_size; 
  IncrCallgraphDFSFrame   *incr_callgraph_dfs;

  //  printf("calling dfs %p\n",call1);
  //    if (call1 -> goal) { printf("*** ");print_subgoal(stddbg,call1->goal);  printf("\n");}

  incr_callgraph_dfs =
    (IncrCallgraphDFSFrame *)  mem_alloc(10000*sizeof(IncrCallgraphDFSFrame), 
					 TABLE_SPACE);
  incr_callgraph_dfs_size = 10000;
  dfs_outedges_check_non_completed(CTXTc call1);
  call1->deleted = 1;
  h=call1->outedges->hasht;
  if (hashtable1_count(h) > 0){
    //    printf("1-call1: %p \n",call1);
    push_dfs_frame(call1,0);
  }
  while (incr_callgraph_dfs_top > -1) {
    itr = 0;
    do {
      if (incr_callgraph_dfs[incr_callgraph_dfs_top].itr) {
	itr = incr_callgraph_dfs[incr_callgraph_dfs_top].itr;
	//	printf("1-top %d itr %p\n",incr_callgraph_dfs_top,itr);
	if (!hashtable1_iterator_advance(itr)) {
	  itr = 0;
	  cn = incr_callgraph_dfs[incr_callgraph_dfs_top].cn;
	  add_callnode(&affected_gl,cn);		
	  //	  printf("+ adding callnode %p\n",cn);
	  pop_dfs_frame;
	}
      }
      else {
	h=incr_callgraph_dfs[incr_callgraph_dfs_top].cn->outedges->hasht;
	if (hashtable1_count(h) > 0) {
	  itr = hashtable1_iterator(h);       
	  incr_callgraph_dfs[incr_callgraph_dfs_top].itr = itr;
	}
	else {
	  cn = incr_callgraph_dfs[incr_callgraph_dfs_top].cn;
	  add_callnode(&affected_gl,cn);		
	  //	  printf("+ adding callnode %p\n",cn);
	  pop_dfs_frame;
	}
	//	printf("2-h %p itr %p\n",h,itr);
      }
    } while (itr == 0 && incr_callgraph_dfs_top > -1);
    if (incr_callgraph_dfs_top > -1) {
      cn = hashtable1_iterator_value(itr);
      //      printf("top %d cn: %p itr: %p\n",incr_callgraph_dfs_top,cn,itr);
      //      if (cn -> goal) {
      //      	printf("*** ");print_subgoal(stddbg,(VariantSF) cn->goal);  printf("\n");
      //            }
      cn->falsecount++;
      if (cn->deleted==0) {
	dfs_outedges_check_non_completed(CTXTc cn);
	cn->deleted = 1;
	//	h=cn->outedges->hasht;
	//	if (hashtable1_count(h) > 0) {
	  push_dfs_frame(cn,0);
	  //	}
      }
    }
  }
  mem_dealloc(incr_callgraph_dfs, incr_callgraph_dfs_size*sizeof(IncrCallgraphDFSFrame), 
	      TABLE_SPACE);
}

/*  
void dfs_outedges(CTXTdeclc callnodeptr call1){
  callnodeptr cn;
  struct hashtable *h;	
  struct hashtable_itr *itr;

  //    if (call1->goal) {
  //      printf("dfs outedges "); print_subgoal(stddbg,call1->goal);printf("\n");
  //    }
  if(IsNonNULL(call1->goal) && !subg_is_completed((VariantSF)call1->goal)){
    dfs_outedges_new_table_error(CTXTc call1);
  }
  call1->deleted = 1;
  h=call1->outedges->hasht;
  
  itr = hashtable1_iterator(h);       
  if (hashtable1_count(h) > -1){
    do {
      cn = hashtable1_iterator_value(itr);
      cn->falsecount++;
      if(cn->deleted==0)
	dfs_outedges(CTXTc cn);
    } while (hashtable1_iterator_advance(itr));
  }
  add_callnode(&affected_gl,call1);		
}
*/
// TLS: factored out this warning because dfs_inedges is recursive and
// this makes the stack frames too big. 
void dfs_inedges_warning(CTXTdeclc callnodeptr call1,calllistptr *lazy_affected) {
  deallocate_call_list(*lazy_affected);
  sprint_subgoal(CTXTc forest_log_buffer_1,0,call1->goal);
    xsb_warn("%d Choice point(s) exist to the table for %s -- cannot incrementally update (dfs_inedges)\n",
	     subg_visitors(call1->goal),forest_log_buffer_1->fl_buffer);
  }



/* If ret != 0 (= CANNOT_UPDATE) then we'll use the old table, and we
   wont lazily update at all. */
int dfs_inedges(CTXTdeclc callnodeptr call1, calllistptr * lazy_affected, int flag ){
  calllistptr inedge_list;
  VariantSF subgoal;
  int ret = 0;

  if(IsNonNULL(call1->goal)) {
    if (!subg_is_completed((VariantSF)call1->goal)){
      deallocate_call_list(*lazy_affected);
      xsb_new_table_error(CTXTc "incremental_tabling",
			  "Incremental tabling is trying to invalidate an incomplete table",
			  get_name(TIF_PSC(subg_tif_ptr(call1->goal))),
			  get_arity(TIF_PSC(subg_tif_ptr(call1->goal))));
    }
    if (subg_visitors(call1->goal)) {
      dfs_inedges_warning(CTXTc call1,lazy_affected);
      return CANNOT_UPDATE;
    }
  }
  // TLS: handles dags&cycles -- no need to traverse more than once.
  if (call1 -> recomputable == COMPUTE_DEPENDENCIES_FIRST)
    call1 -> recomputable = COMPUTE_DIRECTLY;
  else {     //    printf("found directly computable call \n");
    return 0;
  }
  //  printf(" dfs_i affected "); print_subgoal(stddbg,call1->goal);printf("\n");
  inedge_list= call1-> inedges;
  while(IsNonNULL(inedge_list) && !ret){
    subgoal = (VariantSF) inedge_list->inedge_node->callnode->goal;
    if(IsNonNULL(subgoal)){ /* fact check */
      //      count++;
      if (inedge_list->inedge_node->callnode->falsecount > 0)  {
	ret = ret | dfs_inedges(CTXTc inedge_list->inedge_node->callnode, lazy_affected,flag);
      }
      else {
	; //	printf(" dfs_i non_affected "); print_subgoal(stddbg,subgoal);printf("\n");
      }
    }
    inedge_list = inedge_list->next;
  }
  if(IsNonNULL(call1->goal) & !ret){ /* fact check */
    //    printf(" dfs_i adding "); print_subgoal(stddbg,call1->goal);printf("\n");
    add_callnode(lazy_affected,call1);		
  }
  return ret;
}

 
void invalidate_call(CTXTdeclc callnodeptr c){

  //#ifdef MULTI_THREAD
  //  xsb_abort("Incremental Maintenance of tables in not available for multithreaded engine\n");
  //#endif

  if(c->deleted==0){
    c->falsecount++;
    dfs_outedges(CTXTc c);
  }
}

/* to quiet compiler during MT compile, but not clear it will work right for MT... */
extern void print_subgoal(FILE *, VariantSF);

void print_call_list(calllistptr affected_ptr) {

  do {
    printf("item %p sf %p ",calllist_item(affected_ptr),callnode_sf(calllist_item(affected_ptr)));
    printf("next %p ",calllist_next(affected_ptr));  
    if (callnode_sf(calllist_item(affected_ptr)) != NULL) {
      print_subgoal(stddbg, callnode_sf(calllist_item(affected_ptr)));
      }
    printf("\n");
    affected_ptr = (calllistptr) calllist_next(affected_ptr);
  } while ( calllist_next(affected_ptr) != NULL) ;

}

#define WARN_ON_UNSAFE_UPDATE 1

int create_call_list(CTXTdecl){
  callnodeptr call1;
  VariantSF subgoal;
  TIFptr tif;
  int j,count=0,arity;
  Psc psc;
  CPtr oldhreg=NULL;

  //  print_call_list(affected_gl);

  reg[4] = reg[3] = makelist(hreg);  // reg 3 first not-used, use regs in case of stack expanson
  new_heap_free(hreg);   // make heap consistent
  new_heap_free(hreg);
  while((call1 = delete_calllist_elt(&affected_gl)) != EMPTY){
    subgoal = (VariantSF) call1->goal;      
    if(IsNULL(subgoal)){ /* fact predicates */
      call1->deleted = 0; 
      continue;
    }
    //    fprintf(stddbg,"incrementally updating table for ");print_subgoal(stdout,subgoal);printf("\n");
    if (subg_visitors(subgoal)) {
      sprint_subgoal(CTXTc forest_log_buffer_1,0,subgoal);
#ifdef WARN_ON_UNSAFE_UPDATE
      xsb_warn("%d Choice point(s) exist to the table for %s -- cannot incrementally update (create_call_list)\n",
	       subg_visitors(subgoal),forest_log_buffer_1->fl_buffer);
#else
      xsb_abort("%d Choice point(s) exist to the table for %s -- cannot incrementally update (create call list)\n",
	       subg_visitors(subgoal),forest_log_buffer_1->fl_buffer);
#endif
      //      continue;
      break;
    }
    //    fprintf(stddbg,"incrementally updating table for ");print_subgoal(stdout,subgoal);printf("\n");

    count++;
    tif = (TIFptr) subgoal->tif_ptr;
    //    if (!(psc = TIF_PSC(tif)))
    //	xsb_table_error(CTXTc "Cannot access dynamic incremental table\n");	
    psc = TIF_PSC(tif);
    arity = get_arity(psc);
    check_glstack_overflow(4,pcreg,2+arity*200); // don't know how much for build_subgoal_args...
    oldhreg = clref_val(reg[4]);  // maybe updated by re-alloc
    if(arity>0){
      sreg = hreg;
      follow(oldhreg++) = makecs(sreg);
      hreg += arity + 1;  // had 10, why 10?  why not 3? 2 for list, 1 for functor (dsw)
      new_heap_functor(sreg, psc);
      for (j = 1; j <= arity; j++) {
	new_heap_free(sreg);
	cell_array1[arity-j] = cell(sreg-1);
      }
      build_subgoal_args(subgoal);		
    }else{
      follow(oldhreg++) = makestring(get_name(psc));
    }
    reg[4] = follow(oldhreg) = makelist(hreg);
    new_heap_free(hreg);
    new_heap_free(hreg);
  }
  if(count > 0) {
    follow(oldhreg) = makenil;
    hreg -= 2;  /* take back the extra words allocated... */
  } else
    reg[3] = makenil;
    
  //  printterm(stdout,call_list,100);

  return unify(CTXTc reg_term(CTXTc 2),reg_term(CTXTc 3));

  /*
    int i;
    for(i=0;i<callqptr;i++){
      if(IsNonNULL(callq[i]) && (callq[i]->deleted==1)){
    sfPrintGoal(stdout,(VariantSF)callq[i]->goal,NO);
    printf(" %d %d\n",callq[i]->falsecount,callq[i]->deleted);
    }
    }
    printf("-----------------------------\n");
  */
}

//---------------------------------------------------------------------------

int create_lazy_call_list(CTXTdeclc  callnodeptr call1){
  VariantSF subgoal;
  TIFptr tif;
  int j,count=0,arity; 
  Psc psc;
  CPtr oldhreg=NULL;

  //  print_call_list(lazy_affected);

  reg[6] = reg[5] = makelist(hreg);  // reg 5 first not-used, use regs in case of stack expanson
  new_heap_free(hreg);   // make heap consistent
  new_heap_free(hreg);
  while((call1 = delete_calllist_elt(&lazy_affected)) != EMPTY){
    subgoal = (VariantSF) call1->goal;      
    //    fprintf(stddbg,"  considering ");print_subgoal(stdout,subgoal);printf("\n");
    if(IsNULL(subgoal)){ /* fact predicates */
      call1->deleted = 0; 
      continue;
    }
    if (subg_visitors(subgoal)) {
      sprint_subgoal(CTXTc forest_log_buffer_1,0,subgoal);
#ifdef WARN_ON_UNSAFE_UPDATE
      xsb_warn("%d Choice point(s) exist to the table for %s -- cannot incrementally update (create_lazy_call_list)\n",
	       subg_visitors(subgoal),forest_log_buffer_1->fl_buffer);
#else
      xsb_abort("%d Choice point(s) exist to the table for %s -- cannot incrementally update (create_lazy_call_list)\n",
	       subg_visitors(subgoal),forest_log_buffer_1->fl_buffer);
#endif
      continue;
    }
    //    fprintf(stddbg,"adding dependency for ");print_subgoal(stdout,subgoal);printf("\n");

    count++;
    tif = (TIFptr) subgoal->tif_ptr;
    //    if (!(psc = TIF_PSC(tif)))
    //	xsb_table_error(CTXTc "Cannot access dynamic incremental table\n");	
    psc = TIF_PSC(tif);
    arity = get_arity(psc);
    check_glstack_overflow(6,pcreg,2+arity*200); // don't know how much for build_subgoal_args...
    oldhreg = clref_val(reg[6]);  // maybe updated by re-alloc
    if(arity>0){
      sreg = hreg;
      follow(oldhreg++) = makecs(sreg);
      hreg += arity + 1;  // had 10, why 10?  why not 3? 2 for list, 1 for functor (dsw)
      new_heap_functor(sreg, psc);
      for (j = 1; j <= arity; j++) {
	new_heap_free(sreg);
	cell_array1[arity-j] = cell(sreg-1);
      }
      build_subgoal_args(subgoal);		
    } else {
      follow(oldhreg++) = makestring(get_name(psc));
    }
    reg[6] = follow(oldhreg) = makelist(hreg);
    new_heap_free(hreg);
    new_heap_free(hreg);
  }
  if(count > 0) {
    follow(oldhreg) = makenil;
    hreg -= 2;  /* take back the extra words allocated... */
  } else
    reg[5] = makenil;
    
  return unify(CTXTc reg_term(CTXTc 4),reg_term(CTXTc 5));

  /*int i;
    for(i=0;i<callqptr;i++){
      if(IsNonNULL(callq[i]) && (callq[i]->deleted==1)){
    sfPrintGoal(stdout,(VariantSF)callq[i]->goal,NO);
    printf(" %d %d\n",callq[i]->falsecount,callq[i]->deleted);
    }
    }
  printf("-----------------------------\n");   */
}


int in_reg2_list(CTXTdeclc Psc psc) {
  Cell list,term;

  list = reg[2];
  XSB_Deref(list);
  if (isnil(list)) return TRUE; /* if filter is empty, return all */
  while (!isnil(list)) {
    term = get_list_head(list);
    XSB_Deref(term);
    if (isconstr(term)) {
      if (psc == get_str_psc(term)) return TRUE;
    } else if (isstring(term)) {
      if (get_name(psc) == string_val(term)) return TRUE;
    }
    list = get_list_tail(list);
  }
  return FALSE;
}

/* reg 1: tag for this call
   reg 2: filter list of goals to keep (keep all if [])
   reg 3: returned list of changed goals
   reg 4: used as temp (in case of heap expansion)
 */
int create_changed_call_list(CTXTdecl){
  callnodeptr call1;
  VariantSF subgoal;
  TIFptr tif;
  int j, count = 0,arity;
  Psc psc;
  CPtr oldhreg = NULL;

  reg[4] = makelist(hreg);
  new_heap_free(hreg);   // make heap consistent
  new_heap_free(hreg);
  while ((call1 = delete_calllist_elt(&changed_gl)) != EMPTY){
    subgoal = (VariantSF) call1->goal;      
    tif = (TIFptr) subgoal->tif_ptr;
    psc = TIF_PSC(tif);
    if (in_reg2_list(CTXTc psc)) {
      count++;
      arity = get_arity(psc);
      check_glstack_overflow(4,pcreg,2+arity*200); // guess for build_subgoal_args...
      oldhreg = hreg-2;
      if(arity>0){
	sreg = hreg;
	follow(oldhreg++) = makecs(hreg);
	hreg += arity + 1;
	new_heap_functor(sreg, psc);
	for (j = 1; j <= arity; j++) {
	  new_heap_free(sreg);
	  cell_array1[arity-j] = cell(sreg-1);
	}
	build_subgoal_args(subgoal);		
      }else{
	follow(oldhreg++) = makestring(get_name(psc));
      }
      follow(oldhreg) = makelist(hreg);
      new_heap_free(hreg);   // make heap consistent
      new_heap_free(hreg);
    }
  }
  if (count>0)
    follow(oldhreg) = makenil;
  else
    reg[4] = makenil;
    
 
  return unify(CTXTc reg_term(CTXTc 3),reg_term(CTXTc 4));

  /*
    int i;
    for(i=0; i<callqptr; i++){
      if(IsNonNULL(callq[i]) && (callq[i]->deleted==1)){
    sfPrintGoal(stdout,(VariantSF)callq[i]->goal,NO);
    printf(" %d %d\n",callq[i]->falsecount,callq[i]->deleted);
    }
    }
  printf("-----------------------------\n");
  */
}


int immediate_outedges_list(CTXTdeclc callnodeptr call1){
 
  VariantSF subgoal;
  TIFptr tif;
  int j, count = 0,arity;
  Psc psc;
  CPtr oldhreg = NULL;
  struct hashtable *h;	
  struct hashtable_itr *itr;
  callnodeptr cn;
    
  reg[4] = makelist(hreg);
  new_heap_free(hreg);
  new_heap_free(hreg);
  
  if(IsNonNULL(call1)){ /* This can be called from some non incremental predicate */
    h=call1->outedges->hasht;
    
    itr = hashtable1_iterator(h);       
    if (hashtable1_count(h) > 0){
      do {
	cn = hashtable1_iterator_value(itr);
	if(IsNonNULL(cn->goal)){
	  count++;
	  subgoal = (VariantSF) cn->goal;      
	  tif = (TIFptr) subgoal->tif_ptr;
	  psc = TIF_PSC(tif);
	  arity = get_arity(psc);
	  check_glstack_overflow(4,pcreg,2+arity*200); // don't know how much for build_subgoal_args...
	  oldhreg=hreg-2;
	  if(arity>0){
	    sreg = hreg;
	    follow(oldhreg++) = makecs(sreg);
	    hreg += arity + 1;
	    new_heap_functor(sreg, psc);
	    for (j = 1; j <= arity; j++) {
	      new_heap_free(sreg);
	      cell_array1[arity-j] = cell(sreg-1);
	    }
	    build_subgoal_args(subgoal);		
	  }else{
	    follow(oldhreg++) = makestring(get_name(psc));
	  }
	  follow(oldhreg) = makelist(hreg);
	  new_heap_free(hreg);
	  new_heap_free(hreg);
	}
      } while (hashtable1_iterator_advance(itr));
    }
    if (count>0)
      follow(oldhreg) = makenil;
    else
      reg[4] = makenil;
  }else{
    xsb_warn("Called with non-incremental predicate\n");
    reg[4] = makenil;
  }

  //  printterm(stdout,call_list,100);
  return unify(CTXTc reg_term(CTXTc 3),reg_term(CTXTc 4));
}

int get_outedges_num(CTXTdeclc callnodeptr call1) {
  struct hashtable *h;	
  struct hashtable_itr *itr;

  h=call1->outedges->hasht;
  itr = hashtable1_iterator(h);       
  return hashtable1_count(h);
}

int immediate_affects_ptrlist(CTXTdeclc callnodeptr call1){
 
  VariantSF subgoal;
  int count = 0;
  CPtr oldhreg = NULL;
  struct hashtable *h;	
  struct hashtable_itr *itr;
  callnodeptr cn;
    
  reg[4] = makelist(hreg);
  new_heap_free(hreg);
  new_heap_free(hreg);
  
  if(IsNonNULL(call1)){ /* This can be called from some non incremental predicate */
    h=call1->outedges->hasht;
    
    itr = hashtable1_iterator(h);       
    if (hashtable1_count(h) > 0){
      do {
	cn = hashtable1_iterator_value(itr);
	if(IsNonNULL(cn->goal)){
	  count++;
	  subgoal = (VariantSF) cn->goal;      
	  check_glstack_overflow(4,pcreg,2); 
	  oldhreg=hreg-2;
          follow(oldhreg++) = makeint(subgoal);
	  follow(oldhreg) = makelist(hreg);
	  new_heap_free(hreg);
	  new_heap_free(hreg);
	}
      } while (hashtable1_iterator_advance(itr));
    }
    if (count>0)
      follow(oldhreg) = makenil;
    else
      reg[4] = makenil;
  }
  return unify(CTXTc reg_term(CTXTc 3),reg_term(CTXTc 4));
}

int immediate_depends_ptrlist(CTXTdeclc callnodeptr call1){

  VariantSF subgoal;
  int  count = 0;
  CPtr oldhreg = NULL;
  calllistptr cl;

  reg[4] = makelist(hreg);
  new_heap_free(hreg);
  new_heap_free(hreg);
  if(IsNonNULL(call1)){ /* This can be called from some non incremental predicate */
    cl= call1->inedges;
    
    while(IsNonNULL(cl)){
      subgoal = (VariantSF) cl->inedge_node->callnode->goal;    
      if(IsNonNULL(subgoal)){/* fact check */
	count++;
	check_glstack_overflow(4,pcreg,2); 
	oldhreg = hreg-2;
	follow(oldhreg++) = makeint(subgoal);
	follow(oldhreg) = makelist(hreg);
	new_heap_free(hreg);
	new_heap_free(hreg);
      }
      cl=cl->next;
    }
    if (count>0)
      follow(oldhreg) = makenil;
    else
      reg[4] = makenil;
  }
  return unify(CTXTc reg_term(CTXTc 3),reg_term(CTXTc 4));
}


/*
For a callnode call1 returns a Prolog list of callnode on which call1
immediately depends.
*/
int immediate_inedges_list(CTXTdeclc callnodeptr call1){

  VariantSF subgoal;
  TIFptr tif;
  int j, count = 0,arity;
  Psc psc;
  CPtr oldhreg = NULL;
  calllistptr cl;

  reg[4] = makelist(hreg);
  new_heap_free(hreg);
  new_heap_free(hreg);
  if(IsNonNULL(call1)){ /* This can be called from some non incremental predicate */
    cl= call1->inedges;
    
    while(IsNonNULL(cl)){
      subgoal = (VariantSF) cl->inedge_node->callnode->goal;    
      if(IsNonNULL(subgoal)){/* fact check */
	count++;
	tif = (TIFptr) subgoal->tif_ptr;
	psc = TIF_PSC(tif);
	arity = get_arity(psc);
	check_glstack_overflow(4,pcreg,2+arity*200); // don't know how much for build_subgoal_args...
	oldhreg = hreg-2;
	if(arity>0){
	  sreg = hreg;
	  follow(oldhreg++) = makecs(hreg);
	  hreg += arity + 1;
	  new_heap_functor(sreg, psc);
	  for (j = 1; j <= arity; j++) {
	    new_heap_free(sreg);
	    cell_array1[arity-j] = cell(sreg-1);
	  }		
	  build_subgoal_args(subgoal);		
	}else{
	  follow(oldhreg++) = makestring(get_name(psc));
	}
	follow(oldhreg) = makelist(hreg);
	new_heap_free(hreg);
	new_heap_free(hreg);
      }
      cl=cl->next;
    }
    if (count>0)
      follow(oldhreg) = makenil;
    else
      reg[4] = makenil;
  }else{
    xsb_warn("Called with non-incremental predicate\n");
    reg[4] = makenil;
  }
  return unify(CTXTc reg_term(CTXTc 3),reg_term(CTXTc 4));
}


void print_call_node(callnodeptr call1){
  // not implemented
}


void print_call_graph(){
	// not implemented
}


/* Abolish Incremental Calls: 

To abolish a call for an incremental predicate it requires to check
whether any call is dependent on this call or not. If there are calls
that are dependent on this call (not cyclically) then this call should
not be deleted. Otherwise this call is deleted and the calls that are
supporting this call will also be checked for deletion. So deletion of
an incremental call can have a cascading effect.  

As cyclicity check has to be done we have a two phase algorithm to
deal with this problem. In the first phase we mark all the calls that
can be potentially deleted. In the next phase we unmarking the calls -
which should not be deleted. 
*/

void mark_for_incr_abol(callnodeptr);
void check_assumption_list(void);
void delete_calls(CTXTdecl);
call2listptr create_cdbllist(void);

void abolish_incr_call(CTXTdeclc callnodeptr p){

  marked_list_gl=create_cdbllist();

#ifdef INCR_DEBUG1
  printf("marking phase starts\n");
#endif
  
  mark_for_incr_abol(p);
  check_assumption_list();
#ifdef INCR_DEBUG1
  printf("assumption check ends \n");
#endif

  delete_calls(CTXT);


#ifdef INCR_DEBUG1
  printf("delete call ends\n");
#endif
}



/* Double Linked List functions */

call2listptr create_cdbllist(void){
  call2listptr l;
  SM_AllocateStruct(smCall2List,l);
  l->next=l->prev=l;
  l->item=NULL;
  return l;    
}

call2listptr insert_cdbllist(call2listptr cl,callnodeptr n){
  call2listptr l;
  SM_AllocateStruct(smCall2List,l);
  l->next=cl->next;
  l->prev=cl;
  l->item=n;    
  cl->next=l;
  l->next->prev=l;  
  return l;
}

void delete_callnode(call2listptr n){
  n->next->prev=n->prev;
  n->prev->next=n->next;

  return;
}

/*

Input: takes a callnode Output: puts the callnode to the marked list
and sets the deleted bit; puts it to the assumption list if it has any
dependent calls not marked 

*/

void mark_for_incr_abol(callnodeptr c){
  calllistptr in=c->inedges;
  call2listptr markedlistptr;
  callnodeptr c1;

#ifdef INCR_DEBUG1 
      printf("marking ");printcall(c);printf("\n");
#endif

  
  c->deleted=1;
  markedlistptr=insert_cdbllist(marked_list_gl,c);
  if(c->outcount)
    ecall3(&assumption_list_gl,markedlistptr);
    
  while(IsNonNULL(in)){
    c1=in->inedge_node->callnode;
    c1->outcount--;
    if(c1->deleted==0){
      mark_for_incr_abol(c1);
    }
    in=in->next;
  }  
  return;
}


void delete_calls(CTXTdecl){

  call2listptr  n=marked_list_gl->next,temp;
  callnodeptr c;
  VariantSF goal;

  /* first iteration to delete inedges */
  
  while(n!=marked_list_gl){    
    c=n->item;
    if(c->deleted){
      /* facts are not deleted */       
      if(IsNonNULL(c->goal)){
	deleteinedges(c);
      }
    }
    n=n->next;
  }

  /* second iteration is to delete outedges and callnode */

  n=marked_list_gl->next;
  while(n!=marked_list_gl){    
    temp=n->next;
    c=n->item;
    if(c->deleted){
      /* facts are not deleted */       
      if(IsNonNULL(c->goal)){

	goal=c->goal;
	deletecallnode(c);
	
	abolish_table_call(CTXTc goal,ABOLISH_TABLES_DEFAULT);
      }
    }
    SM_DeallocateStruct(smCall2List,n);
    n=temp;
  }
  
  SM_DeallocateStruct(smCall2List,marked_list_gl);
  marked_list_gl=NULL;
  return;
}




void unmark(callnodeptr c){
  callnodeptr c1;
  calllistptr in=c->inedges;

#ifdef INCR_DEBUG1
      printf("unmarking ");printcall(c);printf("\n");
#endif
  
  c->deleted=0;
  while(IsNonNULL(in)){
    c1=in->inedge_node->callnode;
    c1->outcount++;
    if(c1->deleted)
      unmark(c1);
    in=in->next;
  }  
  
  return;
}




void check_assumption_list(void){
  calllistptr tempin,in=assumption_list_gl;
  call2listptr marked_ptr;
  callnodeptr c;

  while(in){
    tempin=in->next;
    marked_ptr=in->item2;
    c=marked_ptr->item;
    if(c->outcount>0){
      delete_callnode(marked_ptr);
      SM_DeallocateStruct(smCall2List,marked_ptr);

#ifdef INCR_DEBUG1
      printf("in check assumption ");printcall(c);printf("\n");
#endif
      
      if(c->deleted)   
	unmark(c);      
    }
    SM_DeallocateStruct(smCallList, in);      
    in=tempin;
  }
  
  
  assumption_list_gl=NULL;
  return;
}

void free_incr_hashtables(TIFptr tif) {
  printf("free incr hash tables not implemented, memory leak\n");
}

extern void *hashtable1_iterator_key(struct hashtable_itr *);

void xsb_compute_scc(SCCNode * ,int * ,int,int *,struct hashtable*,int *,int * );
int return_scc_list(CTXTdeclc SCCNode *, int);

int  get_incr_sccs(CTXTdeclc Cell listterm) {
  Cell orig_listterm, intterm, node;
    long int node_num=0;
    int i = 0, dfn, component = 1;     int * dfn_stack; int dfn_top = 0, ret;
    SCCNode * nodes;
    struct hashtable_itr *itr;     struct hashtable* hasht; 
    XSB_Deref(listterm);
    hasht = create_hashtable1(HASH_TABLE_SIZE, hashid, equalkeys);
    orig_listterm = listterm;
    intterm = get_list_head(listterm);
    XSB_Deref(intterm);
    //    printf("listptr %p @%p\n",listptr,(CPtr) int_val(*listptr));
    insert_some(hasht,(void *) oint_val(intterm),(void *) node_num);
    node_num++; 

    listterm = get_list_tail(listterm);
    XSB_Deref(listterm);
    while (!isnil(listterm)) {
      intterm = get_list_head(listterm);
      XSB_Deref(intterm);
      node = oint_val(intterm);
      if (NULL == search_some(hasht, (void *)node)) {
	insert_some(hasht,(void *)node,(void *)node_num);
	node_num++;
      }
      listterm = get_list_tail(listterm);
      XSB_Deref(listterm);
    }
    nodes = (SCCNode *) mem_calloc(node_num, sizeof(SCCNode),OTHER_SPACE); 
    dfn_stack = (int *) mem_alloc(node_num*sizeof(int),OTHER_SPACE); 
    listterm = orig_listterm;; 
    //printf("listptr %p @%p\n",listptr,(void *)int_val(*(listptr)));
    intterm = get_list_head(listterm);
    XSB_Deref(intterm);
    nodes[0].node = (CPtr) oint_val(intterm);
    listterm = get_list_tail(listterm);
    XSB_Deref(listterm);
    i = 1;
    while (!isnil(listterm)) {
      intterm = get_list_head(listterm);
      XSB_Deref(intterm);
      node = oint_val(intterm);
      nodes[i].node = (CPtr) node;
      listterm = get_list_tail(listterm);
      XSB_Deref(listterm);
      i++;
    }
    itr = hashtable1_iterator(hasht);       
    //    do {
    //      printf("k %p val %p\n",hashtable1_iterator_key(itr),hashtable1_iterator_value(itr));
    //    } while (hashtable1_iterator_advance(itr));

    listterm = orig_listterm;
    //    printf("2: k %p v %p\n",(void *) int_val(*listptr),
    //	   search_some(hasht,(void *) int_val(*listptr)));

    //    while (!isnil(*listptr)) {  now all wrong...
    //      listptr = listptr + 1;
    //      node = int_val(*clref_val(listptr));
    //      printf("2: k %p v %p\n",(CPtr) node,search_some(hasht,(void *) node));
    //      listptr = listptr + 1;
    //    }
    dfn = 1;
    for (i = 0; i < node_num; i++) {
      if (nodes[i].dfn == 0) 
	xsb_compute_scc(nodes,dfn_stack,i,&dfn_top,hasht,&dfn,&component);
      //      printf("++component for node %d is %d (high %d)\n",i,nodes[i].component,component);
    }
    ret = return_scc_list(CTXTc  nodes, node_num);
    hashtable1_destroy(hasht,0);
    mem_dealloc(nodes,node_num*sizeof(SCCNode),OTHER_SPACE); 
    mem_dealloc(dfn_stack,node_num*sizeof(int),OTHER_SPACE); 
    return ret;
}

void xsb_compute_scc(SCCNode * nodes,int * dfn_stack,int node_from, int * dfn_top,
		     struct hashtable* hasht,int * dfn,int * component ) {
  struct hashtable_itr *itr;
  struct hashtable* edges_hash;
  CPtr sf;     int node_to; int j;

  //  printf("xsb_compute_scc for %d %p %s/%d dfn %d dfn_top %d\n",
  //	 node_from,nodes[node_from].node,
  //	 get_name(TIF_PSC(subg_tif_ptr(nodes[node_from].node))),
  //	 get_arity(TIF_PSC(subg_tif_ptr(nodes[node_from].node))),*dfn,*dfn_top);
  nodes[node_from].low = nodes[node_from].dfn = (*dfn)++;
  dfn_stack[*dfn_top] = node_from;
  nodes[node_from].stack = *dfn_top;
  (*dfn_top)++;
  edges_hash = subg_callnode_ptr(nodes[node_from].node)->outedges->hasht;
  itr = hashtable1_iterator(edges_hash);       
  if (hashtable1_count(edges_hash) > 0) {
    //    printf("found %d edges\n",hashtable1_count(edges_hash));
    do {
      sf = ((callnodeptr) hashtable1_iterator_value(itr))-> goal;
      node_to = (int) search_some(hasht, (void *)sf);
      //      printf("edge from %p to %p (%d)\n",(void *)nodes[node_from].node,sf,node_to);
      if (nodes[node_to].dfn == 0) {
	xsb_compute_scc(nodes,dfn_stack,node_to, dfn_top,hasht,dfn,component );
	if (nodes[node_to].low < nodes[node_from].low) 
	  nodes[node_from].low = nodes[node_to].low;
	}	  
	else if (nodes[node_to].dfn < nodes[node_from].dfn  && nodes[node_to].component == 0) {
	  if (nodes[node_to].low < nodes[node_from].low) { nodes[node_from].low = nodes[node_to].low; }
	}
      } while (hashtable1_iterator_advance(itr));
    //    printf("nodes[%d] low %d dfn %d\n",node_from,nodes[node_from].low, nodes[node_from].dfn);
    if (nodes[node_from].low == nodes[node_from].dfn) {
      for (j = (*dfn_top)-1 ; j >= nodes[node_from].stack ; j--) {
	//	printf(" pop %d and assign %d\n",j,*component);
	nodes[dfn_stack[j]].component = *component;
      }
      (*component)++;       *dfn_top = j+1;
    }
  } 
  else nodes[node_from].component = (*component)++;
}


int return_scc_list(CTXTdeclc SCCNode * nodes, int num_nodes){
 
  VariantSF subgoal;
  TIFptr tif;

  int cur_node = 0,arity, j;
  Psc psc;
  CPtr oldhreg = NULL;

  reg[4] = makelist(hreg);
  new_heap_free(hreg);  new_heap_free(hreg);
  do {
    subgoal = (VariantSF) nodes[cur_node].node;
    tif = (TIFptr) subgoal->tif_ptr;
    psc = TIF_PSC(tif);
    arity = get_arity(psc);
    //    printf("subgoal %p, %s/%d\n",subgoal,get_name(psc),arity);
    check_glstack_overflow(4,pcreg,2+arity*200); // don't know how much for build_subgoal_args..
    oldhreg=hreg-2;                          // ptr to car
    if(arity>0){
      sreg = hreg;
      follow(oldhreg++) = makecs(sreg);      
      new_heap_functor(sreg,get_ret_psc(2)); //  car pts to ret/2  psc
      hreg += 3;                             //  hreg pts past ret/2
      sreg = hreg;
      follow(hreg-1) = makeint(nodes[cur_node].component);  // arg 2 of ret/2 pts to component
      follow(hreg-2) = makecs(sreg);         
      new_heap_functor(sreg, psc);           //  arg 1 of ret/2 pts to goal psc
      hreg += arity + 1;
      for (j = 1; j <= arity; j++) {
	new_heap_free(sreg);
	cell_array1[arity-j] = cell(sreg-1);
      }
      build_subgoal_args(subgoal);		
    } else{
      follow(oldhreg++) = makestring(get_name(psc));
    }
    follow(oldhreg) = makelist(hreg);        // cdr points to next car
    new_heap_free(hreg); new_heap_free(hreg);
    cur_node++;
  } while (cur_node  < num_nodes);
  follow(oldhreg) = makenil;                // cdr points to next car
  return unify(CTXTc reg_term(CTXTc 3),reg_term(CTXTc 4));
}

