/* $Header: /data/zender/nco_20150216/nco/src/nco/nco_grp_utl.c,v 1.686 2013-03-28 18:46:38 zender Exp $ */

/* Purpose: Group utilities */

/* Copyright (C) 2011--2013 Charlie Zender
   License: GNU General Public License (GPL) Version 3
   See http://www.gnu.org/copyleft/gpl.html for full license text */

/* Testing:
   eos52nc4 ~/nco/data/in.he5 ~/in.nc4
   export HDF5_DISABLE_VERSION_CHECK=1
   ncdump -h ~/in.nc4
   ncks -D 1 -m ~/in.nc4
   ncks -O -D 3 -g HIRDLS -m ~/in.nc4 ~/foo.nc
   ncks -O -D 3 -m ~/in_grp.nc ~/foo.nc
   ncks -O -D 3 -v 'Q.?' ~/nco/data/in.nc ~/foo.nc */

#include "nco_grp_utl.h"  /* Group utilities */

void
nco_flg_set_grp_var_ass               /* [fnc] Set flags for groups and variables associated with matched object */
(const char * const grp_nm_fll,       /* I [sng] Full name of group */
 const nco_obj_typ obj_typ,           /* I [enm] Object type (group or variable) */
 trv_tbl_sct * const trv_tbl)         /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Set flags for groups and variables associated with matched object */  

  trv_sct trv_obj; /* [sct] Traversal table object */

  for(unsigned int obj_idx=0;obj_idx<trv_tbl->nbr;obj_idx++){
    
    /* Create shallow copy to avoid indirection */
    trv_obj=trv_tbl->lst[obj_idx];
    
    /* If group was user-specified, flag all variables in group */
    if(obj_typ == nco_obj_typ_grp && trv_obj.nco_typ == nco_obj_typ_var)
      if(!strcmp(grp_nm_fll,trv_obj.grp_nm_fll)) trv_tbl->lst[obj_idx].flg_vsg=True;

    /* If variable was user-specified, flag group containing variable */
    if(obj_typ == nco_obj_typ_var && trv_obj.nco_typ == nco_obj_typ_grp)
      if(!strcmp(grp_nm_fll,trv_obj.grp_nm_fll)) trv_tbl->lst[obj_idx].flg_gcv=True;

    /* Flag ancestor groups of all user-specified groups and variables */
    if(strstr(grp_nm_fll,trv_obj.grp_nm_fll)) trv_tbl->lst[obj_idx].flg_ncs=True;

  } /* end loop over obj_idx */
  
} /* end nco_flg_set_grp_var_ass() */

int                                   /* [rcd] Return code */
nco_inq_grps_full                     /* [fnc] Discover and return IDs of apex and all sub-groups */
(const int grp_id,                    /* I [ID] Apex group */
 int * const grp_nbr,                 /* O [nbr] Number of groups */
 int * const grp_ids)                 /* O [ID] Group IDs of children */
{
  /* Purpose: Discover and return IDs of apex and all sub-groups
     If grp_nbr is NULL, it is ignored 
     If grp_ids is NULL, it is ignored
     grp_ids must contain enough space to hold grp_nbr IDs */
  int rcd=NC_NOERR; /* [rcd] Return code */
  int grp_nbr_crr; /* [nbr] Number of groups counted so far */
  int grp_id_crr; /* [ID] Current group ID */

  grp_stk_sct *grp_stk; /* [sct] Group stack pointer */
  
  /* Initialize variables that are incremented */
  grp_nbr_crr=0; /* [nbr] Number of groups counted (i.e., stored in grp_ids array) so far */

  /* Initialize and obtain group iterator */
  rcd+=nco_grp_stk_get(grp_id,&grp_stk);

  /* Find and return next group ID */
  rcd+=nco_grp_stk_nxt(grp_stk,&grp_id_crr);
  while(grp_id_crr != NCO_LST_GRP){
    /* Store last popped value into group ID array */
    if(grp_ids) grp_ids[grp_nbr_crr]=grp_id_crr; /* [ID] Group IDs of children */

    /* Increment counter */
    grp_nbr_crr++;

    /* Find and return next group ID */
    rcd+=nco_grp_stk_nxt(grp_stk,&grp_id_crr);
  } /* end while */
  if(grp_nbr) *grp_nbr=grp_nbr_crr; /* O [nbr] Number of groups */

  /* Free group iterator */
  nco_grp_itr_free(grp_stk);

  return rcd; /* [rcd] Return code */
} /* end nco_inq_grps_full() */

int                                   /* O [rcd] Return code */
nco_def_grp_full                      /* [fnc] Ensure all components of group path are defined */
(const int nc_id,                     /* I [ID] netCDF output-file ID */
 const char * const grp_nm_fll,       /* I [sng] Full group name */
 int * const grp_out_id)              /* O [ID] Deepest group ID */
{
  /* Purpose: Ensure all components of full path name exist and return ID of deepest group */
  char *grp_pth=NULL; /* [sng] Full group path */
  char *grp_pth_dpl=NULL; /* [sng] Full group path memory duplicate */
  char *sls_ptr; /* [sng] Pointer to slash */

  int grp_id_prn; /* [ID] Parent group ID */
  int rcd=NC_NOERR;

  /* Initialize defaults */
  *grp_out_id=nc_id;

  grp_pth_dpl=(char *)strdup(grp_nm_fll);
  grp_pth=grp_pth_dpl;

  /* No need to try to define root group */
  if(grp_pth[0] == '/') grp_pth++;

  /* Define everything necessary beneath root group */
  while(strlen(grp_pth)){
    /* Terminate path at next slash, if any */
    sls_ptr=strchr(grp_pth,'/');

    /* Replace slash by NUL */
    if(sls_ptr) *sls_ptr='\0';
    
    /* Identify parent group */
    grp_id_prn=*grp_out_id;
    
    /* If current group is not defined, define it */
    if(nco_inq_ncid_flg(grp_id_prn,grp_pth,grp_out_id)){
      nco_def_grp(grp_id_prn,grp_pth,grp_out_id);
    }

    /* Point to next group, if any */
    if(sls_ptr) grp_pth=sls_ptr+1; else break;
  } /* end while */

  grp_pth_dpl=(char *)nco_free(grp_pth_dpl);
  return rcd;
}  /* end nco_def_grp_full() */

void
nco_grp_itr_free                      /* [fnc] Free group iterator */
(grp_stk_sct * const grp_stk)         /* O [sct] Group stack pointer */
{
  /* Purpose: Free group iterator
     Call a function that hides the iterator implementation behind the API */
  nco_grp_stk_free(grp_stk);
} /* end nco_grp_itr_free() */

int                                   /* [rcd] Return code */
nco_grp_stk_get                       /* [fnc] Initialize and obtain group iterator */
(const int grp_id,                    /* I [ID] Apex group */
 grp_stk_sct ** const grp_stk)        /* O [sct] Group stack pointer */
{
  /* Purpose: Initialize and obtain group iterator
     Routine pushes apex group onto stack
     The "Apex group" is normally (though not necessarily) the netCDF file ID, 
     aka, root ID, and is the top group in the hierarchy.
     Returned iterator contains one valid group, the apex group,
     and the stack counter equals one */
  int rcd=NC_NOERR; /* [rcd] Return code */
  
  rcd+=nco_inq_grps(grp_id,(int *)NULL,(int *)NULL);

  /* These error codes would cause an abort in the netCDF wrapper layer anyway,
     so this condition is purely for defensive programming. */
  if(rcd != NC_EBADID && rcd != NC_EBADGRPID){
    *grp_stk=nco_grp_stk_ntl();

    /* [fnc] Push group ID onto stack */
    (void)nco_grp_stk_psh(*grp_stk,grp_id);
  } /* endif */

  return rcd; /* [rcd] Return code */
} /* end nco_grp_stk_get() */

int                                   /* [rcd] Return code */
nco_grp_stk_nxt                       /* [fnc] Find and return next group ID */
(grp_stk_sct * const grp_stk,         /* O [sct] Group stack pointer */
 int * const grp_id)                  /* O [ID] Group ID */
{
  /* Purpose: Find and return next group ID
     Next group ID is popped off stack 
     If this group contains sub-groups, these sub-groups are pushed onto stack before return
     Hence routine ensures every group is eventually examined for sub-groups */
  int *grp_ids; /* [ID] Group IDs of children */
  int idx;
  int grp_nbr; /* I [nbr] Number of sub-groups */
  int rcd=NC_NOERR; /* [rcd] Return code */
 
  /* Is stack empty? */
  if(grp_stk->grp_nbr == 0){
    /* Return flag showing iterator has reached the end, i.e., no more groups */
    *grp_id=NCO_LST_GRP;
  }else{
    /* Return current stack top */
    *grp_id=nco_grp_stk_pop(grp_stk);
    /* Replenish stack with sub-group IDs, if available */
    rcd+=nco_inq_grps(*grp_id,&grp_nbr,(int *)NULL);
    if(grp_nbr > 0){
      /* Add sub-groups of current stack top */
      grp_ids=(int *)nco_malloc(grp_nbr*sizeof(int));
      rcd+=nco_inq_grps(*grp_id,(int *)NULL,grp_ids);
      /* Push sub-group IDs in reverse order so when popped will come out in original order */
      for(idx=grp_nbr-1;idx>=0;idx--) (void)nco_grp_stk_psh(grp_stk,grp_ids[idx]);
      /* Clean up memory */
      grp_ids=(int *)nco_free(grp_ids);
    } /* endif sub-groups exist */
  } /* endelse */

  return rcd; /* [rcd] Return code */
} /* end nco_grp_stk_nxt() */

grp_stk_sct *                         /* O [sct] Group stack pointer */
nco_grp_stk_ntl                       /* [fnc] Initialize group stack */
(void)
{
  /* Purpose: Initialize dynamic array implementation of group stack */
  grp_stk_sct *grp_stk; /* O [sct] Group stack pointer */
  grp_stk=(grp_stk_sct *)nco_malloc(sizeof(grp_stk)); /* O [sct] Group stack pointer */
  grp_stk->grp_nbr=0; /* [nbr] Number of items in stack = number of elements in grp_id array */
  grp_stk->grp_id=NULL; /* [ID] Group ID */
  return grp_stk;/* O [sct] Group stack pointer */
} /* end nco_grp_stk_ntl() */

void
nco_grp_stk_psh                       /* [fnc] Push group ID onto stack */
(grp_stk_sct * const grp_stk,         /* I/O [sct] Group stack pointer */
 const int grp_id)                    /* I [ID] Group ID to push */
{
  /* Purpose: Push group ID onto dynamic array implementation of stack */
  grp_stk->grp_nbr++; /* [nbr] Number of items in stack = number of elements in grp_id array */
  grp_stk->grp_id=(int *)nco_realloc(grp_stk->grp_id,(grp_stk->grp_nbr)*sizeof(int)); /* O [sct] Pointer to group IDs */
  grp_stk->grp_id[grp_stk->grp_nbr-1]=grp_id; /* [ID] Group ID */
} /* end nco_grp_stk_psh() */

int /* O [ID] Group ID that was popped */
nco_grp_stk_pop /* [fnc] Remove and return group ID from stack */
(grp_stk_sct * const grp_stk) /* I/O [sct] Pointer to top of stack */
{
  /* Purpose: Remove and return group ID from dynamic array implementation of stack */
  int grp_id; /* [ID] Group ID that was popped */
  grp_id=grp_stk->grp_id[grp_stk->grp_nbr-1]; /* [ID] Group ID that was popped */

  if(grp_stk->grp_nbr == 0){
    (void)fprintf(stderr,"%s: ERROR nco_grp_stk_pop() asked to pop empty stack\n",prg_nm_get());
    nco_exit(EXIT_FAILURE);
  } /* endif */
  grp_stk->grp_nbr--; /* [nbr] Number of items in stack = number of elements in grp_id array */
  grp_stk->grp_id=(int *)nco_realloc(grp_stk->grp_id,(grp_stk->grp_nbr)*sizeof(int)); /* O [sct] Pointer to group IDs */

  return grp_id; /* [ID] Group ID that was popped */
} /* end nco_grp_stk_pop() */

void
nco_grp_stk_free                      /* [fnc] Free group stack */
(grp_stk_sct * const grp_stk)         /* O [sct] Group stack pointer */
{
  /* Purpose: Free dynamic array implementation of stack */
  grp_stk->grp_id=(int *)nco_free(grp_stk->grp_id);
} /* end nco_grp_stk_free() */

void
nco_prt_grp_nm_fll                    /* [fnc] Debug function to print group full name from ID */
(const int grp_id)                    /* I [ID] Group ID */
{
  size_t grp_nm_lng;
  char *grp_nm_fll;

#if defined(HAVE_NETCDF4_H) 
  (void)nco_inq_grpname_full(grp_id, &grp_nm_lng, NULL);
  grp_nm_fll=(char*)nco_malloc(grp_nm_lng+1L);
  (void)nco_inq_grpname_full(grp_id, &grp_nm_lng, grp_nm_fll);
  (void)fprintf(stdout,"<%s>",grp_nm_fll);
  grp_nm_fll=(char*)nco_free(grp_nm_fll);
#endif

} /* nco_inq_prt_nm_fll() */

int                                  /* [rcd] Return code */
nco_grp_dfn                          /* [fnc] Define groups in output file */
(const int out_id,                   /* I [ID] netCDF output-file ID */
 nm_id_sct *grp_xtr_lst,             /* [grp] Groups to be defined */
 const int grp_nbr)                  /* I [nbr] Number of groups to be defined */
{
  /* Purpose: Define groups in output file */
  int idx;
  int rcd=NC_NOERR; /* [rcd] Return code */
  int rcr_lvl=1; /* [nbr] Recursion level */

  if(dbg_lvl_get() >= nco_dbg_scl) (void)fprintf(stderr,"%s: INFO nco_grp_dfn() reports file level = 0 parent group = / (root group) will have %d sub-group%s\n",prg_nm_get(),grp_nbr,(grp_nbr == 1) ? "" : "s");

  /* For each (possibly user-specified) top-level group ... */
  for(idx=0;idx<grp_nbr;idx++){
    /* Define group and all subgroups */
    rcd+=nco_def_grp_rcr(grp_xtr_lst[idx].id,out_id,grp_xtr_lst[idx].nm,rcr_lvl);
  } /* end loop over top-level groups */

  return rcd;
} /* end nco_grp_dfn() */

int                                   /* [rcd] Return code */
nco_def_grp_rcr                       /* [fnc] Define groups */
(const int in_id,                     /* I [ID] netCDF input-file ID */
 const int out_id,                    /* I [ID] netCDF output-file ID */
 const char * const prn_nm,           /* I [sng] Parent group name */
 const int rcr_lvl)                   /* I [nbr] Recursion level */
{
  /* Purpose: Recursively define parent group and all subgroups in output file */
  char grp_nm[NC_MAX_NAME];

  int idx;
  int grp_nbr; /* I [nbr] Number of sub-groups */
  int rcd=NC_NOERR; /* [rcd] Return code */
  int grp_in_ids[NC_MAX_DIMS]; /* [ID] Sub-group IDs in input file */ /* fxm: NC_MAX_GRPS? */
  int grp_out_ids[NC_MAX_DIMS]; /* [ID] Sub-group IDs in output file */

  /* How many and which sub-groups are in this group? */
  rcd+=nco_inq_grps(in_id,&grp_nbr,grp_in_ids);

  if(dbg_lvl_get() >= nco_dbg_scl) (void)fprintf(stderr,"%s: INFO nco_def_grp_rcr() reports file level = %d parent group = %s will have %d sub-group%s\n",prg_nm_get(),rcr_lvl,prn_nm,grp_nbr,(grp_nbr == 1) ? "" : "s");

  /* Define each group, recursively, in output file */
  for(idx=0;idx<grp_nbr;idx++){
    /* Obtain name of current group in input file */
    rcd+=nco_inq_grpname(grp_in_ids[idx],grp_nm);

    /* Define group of same name in output file */
    rcd+=nco_def_grp(out_id,grp_nm,grp_out_ids+idx);

    /* Define group and all sub-groups */
    rcd+=nco_def_grp_rcr(grp_in_ids[idx],grp_out_ids[idx],grp_nm,rcr_lvl+1);
  } /* end loop over sub-groups groups */

  return rcd;
} /* end nco_grp_dfn_rcr() */

int
nco_get_sls_chr_cnt                   /* [fnc] Get number of slash characterrs in a string path  */
(char * const nm_fll)                 /* I [sct] Full name  */
{
  char *ptr_chr;      /* [sng] Pointer to character '/' in full name */
  int nbr_sls_chr=0;  /* [nbr] Number of of slash characterrs in  string path */
  int psn_chr;        /* [nbr] Position of character '/' in in full name */
 
  if(dbg_lvl_get() == 14) (void)fprintf(stdout,"Looking '/' in \"%s\"...",nm_fll);

  ptr_chr=strchr(nm_fll,'/');
  while (ptr_chr!=NULL)
  {
    psn_chr=ptr_chr-nm_fll;

    if(dbg_lvl_get() == 14) (void)fprintf(stdout," ::found at %d",psn_chr);

    ptr_chr=strchr(ptr_chr+1,'/');

    nbr_sls_chr++;
  }

  if(dbg_lvl_get() == 14) (void)fprintf(stdout,"\n");
  return nbr_sls_chr;

} /* nco_get_sls_chr_cnt() */



int
nco_get_str_pth_sct                   /* [fnc] Get full name token structure (path components) */
(char * const nm_fll,                 /* I [sng] Full name  */ 
 str_pth_sct ***str_pth_lst)          /* I/O [sct] List of path components  */    
{
  /* Purpose: Break a full path name into components separated by the slash character (netCDF4 path separator) 
  
  strtok()
  A sequence of calls to this function split str into tokens, which are sequences of contiguous characters 
  separated by any of the characters that are part of delimiters.

  strchr() is used to get position of separator that corresponsds to each token

  Use case: "/g16/g16g1/lon1"

  Token 0: g16
  Token 1: g16g1
  Token 2: lon1

  Usage

  Get number of tokens in variable full name
  nbr_sls_chr_var=nco_get_sls_chr_cnt(var_trv->nm_fll); 

  Alloc
  str_pth_lst_var=(str_pth_sct **)nco_malloc(nbr_sls_chr_var*sizeof(str_pth_sct *)); 

  Get token list in variable full name 
  (void)nco_get_str_pth_sct(var_trv->nm_fll,&str_pth_lst_var); 
  
  */

  char *ptr_chr;      /* [sng] Pointer to character '/' in full name */
  char *ptr_chr_tok;  /* [sng] Pointer to character */
  int nbr_sls_chr=0;  /* [nbr] Number of of slash characterrs in  string path */
  int psn_chr;        /* [nbr] Position of character '/' in in full name */
 
  /* Duplicate original, since strtok() changes it */
  char *str=strdup(nm_fll);

  if(dbg_lvl_get() == 14) (void)fprintf(stdout,"Splitting \"%s\" into tokens:\n",str);

  /* Get first token */
  ptr_chr_tok=strtok (str,"/");

  ptr_chr=strchr(nm_fll,'/');

  while (ptr_chr!=NULL)
  {
    if(dbg_lvl_get() == 14) (void)fprintf(stdout,"#%s ",ptr_chr_tok);

    psn_chr=ptr_chr-nm_fll;
    
    /* Store token and position */
    (*str_pth_lst)[nbr_sls_chr]=(str_pth_sct *)nco_malloc(1*sizeof(str_pth_sct));

    (*str_pth_lst)[nbr_sls_chr]->nm=strdup(ptr_chr_tok);
    (*str_pth_lst)[nbr_sls_chr]->psn=psn_chr;

    /* The point where the last token was found is kept internally by the function */
    ptr_chr_tok = strtok (NULL, "/");

    ptr_chr=strchr(ptr_chr+1,'/');

    nbr_sls_chr++;   
  }

  if(dbg_lvl_get() == 14)(void)fprintf(stdout,"\n");

  str=(char *)nco_free(str);

  return nbr_sls_chr;

} /* nco_get_sls_chr_cnt() */



void 
nco_prn_att_trv /* [fnc] Traverse tree to print all group and global attributes */
(const int nc_id, /* I [id] netCDF file ID */
 const trv_tbl_sct * const trv_tbl) /* I [sct] GTT (Group Traversal Table) */
{
  int grp_id;                 /* [ID] Group ID */
  int nbr_att;                /* [nbr] Number of attributes */
  int nbr_dmn;                /* [nbr] Number of dimensions */
  int nbr_var;                /* [nbr] Number of variables */

  for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++){
    trv_sct trv=trv_tbl->lst[uidx];
    if(trv.nco_typ == nco_obj_typ_grp){
      /* Obtain group ID from netCDF API using full group name */
      (void)nco_inq_grp_full_ncid(nc_id,trv.nm_fll,&grp_id);

      /* Obtain info for group */
      (void)nco_inq(grp_id,&nbr_dmn,&nbr_var,&nbr_att,(int *)NULL);

      /* List attributes using obtained group ID */
      if(nbr_att){
        if(trv.grp_dpt > 0) (void)fprintf(stdout,"Group %s attributes:\n",trv.nm_fll); else (void)fprintf(stdout,"Global attributes:\n"); 
        (void)nco_prn_att(nc_id,grp_id,NC_GLOBAL); 
      } /* nbr_att */
    } /* end nco_obj_typ_grp */
  } /* end uidx */
} /* end nco_prn_att_trv() */


nco_bool                              /* O [flg] All names are in file */
nco_xtr_mk                            /* [fnc] Check -v and -g input names and create extraction list */
(char ** grp_lst_in,                  /* I [sng] User-specified list of groups */
 const int grp_xtr_nbr,               /* I [nbr] Number of groups in list */
 char ** var_lst_in,                  /* I [sng] User-specified list of variables */
 const int var_xtr_nbr,               /* I [nbr] Number of variables in list */
 const nco_bool EXTRACT_ALL_COORDINATES, /* I [flg] Process all coordinates */ 
 const nco_bool flg_unn,              /* I [flg] Select union of specified groups and variables */
 trv_tbl_sct * const trv_tbl)         /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Verify all user-specified objects exist in file and create extraction list
     Verify both variables and groups
     Handle regular expressions 
     Set flags in traversal table to help generate extraction list

     Tests:
     ncks -O -D 5 -g / ~/nco/data/in_grp.nc ~/foo.nc
     ncks -O -D 5 -g '' ~/nco/data/in_grp.nc ~/foo.nc
     ncks -O -D 5 -g g1 ~/nco/data/in_grp.nc ~/foo.nc
     ncks -O -D 5 -g /g1 ~/nco/data/in_grp.nc ~/foo.nc
     ncks -O -D 5 -g /g1/g1 ~/nco/data/in_grp.nc ~/foo.nc
     ncks -O -D 5 -g g1.+ ~/nco/data/in_grp.nc ~/foo.nc
     ncks -O -D 5 -v v1 ~/nco/data/in_grp.nc ~/foo.nc
     ncks -O -D 5 -g g1.+ -v v1,sc. ~/nco/data/in_grp.nc ~/foo.nc
     ncks -O -D 5 -v scl,/g1/g1g1/v1 ~/nco/data/in_grp.nc ~/foo.nc
     ncks -O -D 5 -g g3g.+,g9/ -v scl,/g1/g1g1/v1 ~/nco/data/in_grp.nc ~/foo.nc */
  
  const char fnc_nm[]="nco_xtr_mk()"; /* [sng] Function name */
  const char sls_chr='/'; /* [chr] Slash character */
  
  char **obj_lst_in; /* [sng] User-specified list of objects */

  char *sbs_srt; /* [sng] Location of user-string match start in object path */
  char *sbs_end; /* [sng] Location of user-string match end   in object path */
  char *usr_sng; /* [sng] User-supplied object name */
  char *var_mch_srt; /* [sng] Location of variable short name in user-string */
  
  int obj_nbr; /* [nbr] Number of objects in list */

#ifdef NCO_HAVE_REGEX_FUNCTIONALITY
  int rx_mch_nbr;
#endif /* !NCO_HAVE_REGEX_FUNCTIONALITY */

  nco_bool flg_ncr_mch_crr; /* [flg] Current group meets anchoring properties of this user-supplied string */
  nco_bool flg_ncr_mch_grp; /* [flg] User-supplied string anchors at root */
  nco_bool flg_pth_end_bnd; /* [flg] String ends   at path component boundary */
  nco_bool flg_pth_srt_bnd; /* [flg] String begins at path component boundary */
  nco_bool flg_rcr_mch_crr; /* [flg] Current group meets recursion criteria of this user-supplied string */
  nco_bool flg_rcr_mch_grp; /* [flg] User-supplied string will match groups recursively */
  nco_bool flg_unn_ffc; /* [flg] Union or Effective Union (not intersection) */
  nco_bool flg_usr_mch_obj; /* [flg] One or more objects match each user-supplied string */
  nco_bool flg_var_cnd; /* [flg] Match meets addition condition(s) for variable */

  nco_obj_typ obj_typ; /* [enm] Object type (group or variable) */
   
  size_t usr_sng_lng; /* [nbr] Length of user-supplied string */

  trv_sct grp_obj; /* [sct] Traversal table object assumed to be group */
  trv_sct trv_obj; /* [sct] Traversal table object */
  trv_sct var_obj; /* [sct] Traversal table object assumed to be variable */

  if(dbg_lvl_get() == nco_dbg_old) (void)fprintf(stdout,"%s: INFO %s Extraction list will be formed as %s of group and variable specifications if both are given\n",prg_nm_get(),fnc_nm,(flg_unn) ? "union" : "intersection");

  /* Specifying no groups or variables is equivalent to requesting all */
  if(!grp_xtr_nbr && !var_xtr_nbr && !EXTRACT_ALL_COORDINATES)
    for(unsigned int obj_idx=0;obj_idx<trv_tbl->nbr;obj_idx++) trv_tbl->lst[obj_idx].flg_dfl=True;

  for(unsigned int itr_idx=0;itr_idx<2;itr_idx++){
    
    /* Set object type (group or variable) */
    if(itr_idx == 0){
      obj_typ=nco_obj_typ_grp;
      obj_lst_in=(char **)grp_lst_in;
      obj_nbr=grp_xtr_nbr;
    }else if(itr_idx == 1){
      obj_typ=nco_obj_typ_var;
      obj_lst_in=(char **)var_lst_in;
      obj_nbr=var_xtr_nbr;
    } /* endelse */
    
    for(int obj_idx=0;obj_idx<obj_nbr;obj_idx++){
      
      /* Initialize state for current user-specified string */
      flg_ncr_mch_grp=False;
      flg_rcr_mch_grp=True;
      flg_usr_mch_obj=False;

      if(!obj_lst_in[obj_idx]){
        (void)fprintf(stderr,"%s: ERROR %s reports user-supplied %s name is empty\n",prg_nm_get(),fnc_nm,(obj_typ == nco_obj_typ_grp) ? "group" : "variable");
        nco_exit(EXIT_FAILURE);
      } /* end else */

      usr_sng=strdup(obj_lst_in[obj_idx]); 
      usr_sng_lng=strlen(usr_sng);

      /* Turn-off recursion for groups? */
      if(obj_typ == nco_obj_typ_grp)
        if(usr_sng_lng > 1L && usr_sng[usr_sng_lng-1L] == sls_chr){
          /* Remove trailing slash for subsequent searches since canonical group names do not end with slash */
          flg_rcr_mch_grp=False;
          usr_sng[usr_sng_lng-1L]='\0';
          usr_sng_lng--;
        } /* flg_rcr_mch_grp */

        /* Turn-on root-anchoring for groups? */
        if(obj_typ == nco_obj_typ_grp)
          if(usr_sng[0L] == sls_chr)
            flg_ncr_mch_grp=True;

        /* Convert pound signs (back) to commas */
        nco_hash2comma(usr_sng);

        /* If usr_sng is regular expression ... */
        if(strpbrk(usr_sng,".*^$\\[]()<>+?|{}")){
          /* ... and regular expression library is present */
#ifdef NCO_HAVE_REGEX_FUNCTIONALITY
          if((rx_mch_nbr=nco_trv_rx_search(usr_sng,obj_typ,trv_tbl))) flg_usr_mch_obj=True;
          if(!rx_mch_nbr) (void)fprintf(stdout,"%s: WARNING: Regular expression \"%s\" does not match any %s\nHINT: See regular expression syntax examples at http://nco.sf.net/nco.html#rx\n",prg_nm_get(),usr_sng,(obj_typ == nco_obj_typ_grp) ? "group" : "variable"); 
          continue;
#else /* !NCO_HAVE_REGEX_FUNCTIONALITY */
          (void)fprintf(stdout,"%s: ERROR: Sorry, wildcarding (extended regular expression matches to variables) was not built into this NCO executable, so unable to compile regular expression \"%s\".\nHINT: Make sure libregex.a is on path and re-build NCO.\n",prg_nm_get(),usr_sng);
          nco_exit(EXIT_FAILURE);
#endif /* !NCO_HAVE_REGEX_FUNCTIONALITY */
        } /* end if regular expression */

        /* usr_sng is not rx, so manually search for multicomponent matches */
        for(unsigned int tbl_idx=0;tbl_idx<trv_tbl->nbr;tbl_idx++){

          /* Create shallow copy to avoid indirection */
          trv_obj=trv_tbl->lst[tbl_idx];

          if(trv_obj.nco_typ == obj_typ){

            /* Initialize defaults for current candidate path to match */
            flg_pth_srt_bnd=False;
            flg_pth_end_bnd=False;
            flg_var_cnd=False;
            flg_rcr_mch_crr=True;
            flg_ncr_mch_crr=True;

            /* Look for partial match, not necessarily on path boundaries */
            if((sbs_srt=strstr(trv_obj.nm_fll,usr_sng))){

              /* Ensure match spans (begins and ends on) whole path-component boundaries */

              /* Does match begin at path component boundary ... directly on a slash? */
              if(*sbs_srt == sls_chr) flg_pth_srt_bnd=True;

              /* ...or one after a component boundary? */
              if((sbs_srt > trv_obj.nm_fll) && (*(sbs_srt-1L) == sls_chr)) flg_pth_srt_bnd=True;

              /* Does match end at path component boundary ... directly on a slash? */
              sbs_end=sbs_srt+usr_sng_lng-1L;

              if(*sbs_end == sls_chr) flg_pth_end_bnd=True;

              /* ...or one before a component boundary? */
              if(sbs_end <= trv_obj.nm_fll+trv_obj.nm_fll_lng-1L)
                if((*(sbs_end+1L) == sls_chr) || (*(sbs_end+1L) == '\0'))
                  flg_pth_end_bnd=True;

              /* Additional condition for variables is user-supplied string must end with short form of variable name */
              if(obj_typ == nco_obj_typ_var){
                if(trv_obj.nm_lng <= usr_sng_lng){
                  var_mch_srt=usr_sng+usr_sng_lng-trv_obj.nm_lng;
                  if(!strcmp(var_mch_srt,trv_obj.nm)) flg_var_cnd=True;
                } /* endif */
                if(dbg_lvl_get() >= nco_dbg_sbr) (void)fprintf(stderr,"%s: INFO %s reports variable %s %s additional conditions for variable match with %s.\n",prg_nm_get(),fnc_nm,usr_sng,(flg_var_cnd) ? "meets" : "fails",trv_obj.nm_fll);
              } /* endif var */

              /* If anchoring, match must begin at root */
              if(flg_ncr_mch_grp && *sbs_srt != sls_chr) flg_ncr_mch_crr=False;

              /* If no recursion, match must terminate user-supplied string */
              if(!flg_rcr_mch_grp && *(sbs_end+1L)) flg_rcr_mch_crr=False;

              /* Set traversal table flags */
              if(obj_typ == nco_obj_typ_var){
                /* Variables must meet necessary flags for variables */
                if(flg_pth_srt_bnd && flg_pth_end_bnd && flg_var_cnd){
                  trv_tbl->lst[tbl_idx].flg_mch=True;
                  trv_tbl->lst[tbl_idx].flg_rcr=False;
                  /* Was matched variable specified as full path (i.e., beginning with slash?) */
                  if(*usr_sng == sls_chr) trv_tbl->lst[tbl_idx].flg_vfp=True;
                } /* end flags */
              }else{ /* !nco_obj_typ_var */
                /* Groups must meet necessary flags for groups */
                if(flg_pth_srt_bnd && flg_pth_end_bnd && flg_ncr_mch_crr && flg_rcr_mch_crr){
                  trv_tbl->lst[tbl_idx].flg_mch=True;
                  trv_tbl->lst[tbl_idx].flg_rcr=flg_rcr_mch_grp;
                } /* end flags */
              }  /* !nco_obj_typ_var */
              /* Set flags for groups and variables associated with this object */
              if(trv_tbl->lst[tbl_idx].flg_mch) nco_flg_set_grp_var_ass(trv_obj.grp_nm_fll,obj_typ,trv_tbl);

              /* Set function return condition */
              if(trv_tbl->lst[tbl_idx].flg_mch) flg_usr_mch_obj=True;

              if(dbg_lvl_get() == nco_dbg_old){
                (void)fprintf(stderr,"%s: INFO %s reports %s %s matches filepath %s. Begins on boundary? %s. Ends on boundary? %s. Extract? %s.",prg_nm_get(),fnc_nm,(obj_typ == nco_obj_typ_grp) ? "group" : "variable",usr_sng,trv_obj.nm_fll,(flg_pth_srt_bnd) ? "Yes" : "No",(flg_pth_end_bnd) ? "Yes" : "No",(trv_tbl->lst[tbl_idx].flg_mch) ?  "Yes" : "No");
                if(obj_typ == nco_obj_typ_grp) (void)fprintf(stderr," Anchored? %s.",(flg_ncr_mch_grp) ? "Yes" : "No");
                if(obj_typ == nco_obj_typ_grp) (void)fprintf(stderr," Recursive? %s.",(trv_tbl->lst[tbl_idx].flg_rcr) ? "Yes" : "No");
                if(obj_typ == nco_obj_typ_grp) (void)fprintf(stderr," flg_gcv? %s.",(trv_tbl->lst[tbl_idx].flg_gcv) ? "Yes" : "No");
                if(obj_typ == nco_obj_typ_grp) (void)fprintf(stderr," flg_ncs? %s.",(trv_tbl->lst[tbl_idx].flg_ncs) ? "Yes" : "No");
                if(obj_typ == nco_obj_typ_var) (void)fprintf(stderr," flg_vfp? %s.",(trv_tbl->lst[tbl_idx].flg_vfp) ? "Yes" : "No");
                if(obj_typ == nco_obj_typ_var) (void)fprintf(stderr," flg_vsg? %s.",(trv_tbl->lst[tbl_idx].flg_vsg) ? "Yes" : "No");
                (void)fprintf(stderr,"\n");
              } /* end if */

            } /* endif strstr() */
          } /* endif nco_obj_typ */
        } /* end loop over tbl_idx */

        if(!flg_usr_mch_obj){
          (void)fprintf(stderr,"%s: ERROR %s reports user-supplied %s name or regular expression %s is not in and/or does not match contents of input file\n",prg_nm_get(),fnc_nm,(obj_typ == nco_obj_typ_grp) ? "group" : "variable",usr_sng);
          nco_exit(EXIT_FAILURE);
        } /* flg_usr_mch_obj */
        /* Free dynamic memory */
        if(usr_sng) usr_sng=(char *)nco_free(usr_sng);

    } /* obj_idx */

    if(dbg_lvl_get() >= nco_dbg_sbr){
      (void)fprintf(stdout,"%s: INFO %s reports following %s match sub-setting and regular expressions:\n",prg_nm_get(),fnc_nm,(obj_typ == nco_obj_typ_grp) ? "groups" : "variables");
      trv_tbl_prn_flg_mch(trv_tbl,obj_typ);
    } /* endif dbg */

  } /* itr_idx */

  /* Compute intersection of groups and variables if necessary
  Intersection criteria flag, flg_nsx, is satisfied by default. Unset later when necessary. */
  for(unsigned int obj_idx=0;obj_idx<trv_tbl->nbr;obj_idx++) trv_tbl->lst[obj_idx].flg_nsx=True;

  /* Union is same as intersection if either variable or group list is empty, otherwise check intersection criteria */
  if(flg_unn || !grp_xtr_nbr || !var_xtr_nbr) flg_unn_ffc=True; else flg_unn_ffc=False;
  if(!flg_unn_ffc){
    for(unsigned int obj_idx=0;obj_idx<trv_tbl->nbr;obj_idx++){
      var_obj=trv_tbl->lst[obj_idx];
      if(var_obj.nco_typ == nco_obj_typ_var){
        /* Cancel (non-full-path) variable match unless variable is also in user-specified group */
        if(var_obj.flg_mch && !var_obj.flg_vfp){
          for(unsigned int obj2_idx=0;obj2_idx<trv_tbl->nbr;obj2_idx++){
            grp_obj=trv_tbl->lst[obj2_idx];
            if(grp_obj.nco_typ == nco_obj_typ_grp && !strcmp(var_obj.grp_nm_fll,grp_obj.grp_nm_fll)) break;
          } /* end loop over obj2_idx */
          if(!grp_obj.flg_mch) trv_tbl->lst[obj_idx].flg_nsx=False;
        } /* flg_mch && flg_vfp */
      } /* nco_obj_typ_grp */
    } /* end loop over obj_idx */
  } /* flg_unn */

  /* Combine previous flags into initial extraction flag */
  for(unsigned int obj_idx=0;obj_idx<trv_tbl->nbr;obj_idx++){
    /* Extract object if ... */
    if(
      (flg_unn_ffc && trv_tbl->lst[obj_idx].flg_mch) || /* ...union mode object matches user-specified string... */
      (flg_unn_ffc && trv_tbl->lst[obj_idx].flg_vsg) || /* ...union mode variable selected because group matches... */
      (flg_unn_ffc && trv_tbl->lst[obj_idx].flg_gcv) || /* ...union mode contains matched variable... */
      (trv_tbl->lst[obj_idx].flg_dfl) || /* ...user-specified no sub-setting... */
      (!flg_unn_ffc && trv_tbl->lst[obj_idx].flg_mch && trv_tbl->lst[obj_idx].flg_nsx) || /* ...intersection mode variable matches group... */
      False) 
      trv_tbl->lst[obj_idx].flg_xtr=True;
  } /* end loop over obj_idx */

  if(dbg_lvl_get() == nco_dbg_old){
    for(unsigned int obj_idx=0;obj_idx<trv_tbl->nbr;obj_idx++){
      /* Create shallow copy to avoid indirection */
      trv_obj=trv_tbl->lst[obj_idx];

      (void)fprintf(stderr,"%s: INFO %s final flags of %s %s:\n",prg_nm_get(),fnc_nm,(trv_obj.nco_typ == nco_obj_typ_grp) ? "group" : "variable",trv_obj.nm_fll);
      (void)fprintf(stderr," flg_dfl? %s.",(trv_obj.flg_dfl) ? "Yes" : "No");
      (void)fprintf(stderr," flg_mch? %s.",(trv_obj.flg_mch) ? "Yes" : "No");
      (void)fprintf(stderr," flg_xtr? %s.",(trv_obj.flg_xtr) ? "Yes" : "No");
      if(trv_obj.nco_typ == nco_obj_typ_var) (void)fprintf(stderr," flg_vfp? %s.",(trv_obj.flg_vfp) ? "Yes" : "No");
      if(trv_obj.nco_typ == nco_obj_typ_var) (void)fprintf(stderr," flg_vsg? %s.",(trv_obj.flg_vsg) ? "Yes" : "No");
      if(trv_obj.nco_typ == nco_obj_typ_grp) (void)fprintf(stderr," flg_gcv? %s.",(trv_obj.flg_gcv) ? "Yes" : "No");
      if(trv_obj.nco_typ == nco_obj_typ_grp) (void)fprintf(stderr," flg_ncs? %s.",(trv_obj.flg_ncs) ? "Yes" : "No");
      (void)fprintf(stderr,"\n");
    } /* end loop over obj_idx */
  } /* endif dbg */

  /* Print extraction list in debug mode */
  if(dbg_lvl_get() == 13) (void)trv_tbl_prn_xtr(trv_tbl,fnc_nm);

  return (nco_bool)True;

} /* end nco_xtr_mk() */


void
nco_xtr_xcl                           /* [fnc] Convert extraction list to exclusion list */
(trv_tbl_sct * const trv_tbl)         /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Convert extraction list to exclusion list */

  const char fnc_nm[]="nco_xtr_xcl()"; /* [sng] Function name */

  for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++)
    if(trv_tbl->lst[uidx].nco_typ == nco_obj_typ_var) 
      trv_tbl->lst[uidx].flg_xtr=!trv_tbl->lst[uidx].flg_xtr;

  /* Print extraction list in debug mode */
  if(dbg_lvl_get() == 13) (void)trv_tbl_prn_xtr(trv_tbl,fnc_nm);

  return;
} /* end nco_xtr_xcl() */

void
nco_xtr_crd_add                       /* [fnc] Add all coordinates to extraction list */
(trv_tbl_sct * const trv_tbl)         /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Add all coordinates to extraction list
  Find all coordinates (variables with same names and sizes as dimensions) and
  ensure they are marked for extraction */

  const char fnc_nm[]="nco_xtr_crd_add()"; /* [sng] Function name */

  /* Loop table */
  for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){

    /* Filter variables  */
    if(trv_tbl->lst[var_idx].nco_typ == nco_obj_typ_var){
      trv_sct var_trv=trv_tbl->lst[var_idx]; 

      /* If variable is coordinate variable then mark it for extraction ...simple */
      if (var_trv.is_crd_var){
        trv_tbl->lst[var_idx].flg_xtr=True;
      }

    } /* Filter variables  */
  } /* Loop table */

  /* Print extraction list in debug mode */
  if(dbg_lvl_get() == 13) (void)trv_tbl_prn_xtr(trv_tbl,fnc_nm);

  return;
} /* end nco_xtr_crd_add() */

void
nco_xtr_cf_add                        /* [fnc] Add to extraction list variables associated with CF convention */
(const int nc_id,                     /* I [ID] netCDF file ID */
 const char * const cf_nm,            /* I [sng] CF convention ("coordinates" or "bounds") */
 trv_tbl_sct * const trv_tbl)         /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Add to extraction list all variables associated with specified CF convention
     Driver routine for nco_xtr_cf_prv_add()
     Detect associated coordinates specified by CF "bounds" and "coordinates" conventions
     http://cf-pcmdi.llnl.gov/documents/cf-conventions/1.1/cf-conventions.html#coordinate-system */ 

  const char fnc_nm[]="nco_xtr_cf_add()"; /* [sng] Function name */

  /* Search for and add CF-compliant bounds and coordinates to extraction list */
  for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++){
    trv_sct trv=trv_tbl->lst[uidx];
    if(trv.nco_typ == nco_obj_typ_var && trv.flg_xtr) (void)nco_xtr_cf_prv_add(nc_id,&trv,cf_nm,trv_tbl);
  } /* end loop over table */


  /* Print extraction list in debug mode */
  if(dbg_lvl_get() == 13) (void)trv_tbl_prn_xtr(trv_tbl,fnc_nm);

  return;
} /* nco_xtr_cf_add() */

void
nco_xtr_cf_prv_add                    /* [fnc] Add specified CF-compliant coordinates of specified variable to extraction list */
(const int nc_id,                     /* I [ID] netCDF file ID */
 const trv_sct * const var_trv,       /* I [sct] Variable (object) */
 const char * const cf_nm,            /* I [sng] CF convention ( "coordinates" or "bounds") */
 trv_tbl_sct * const trv_tbl)         /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Detect associated coordinates specified by CF "bounds" or "coordinates" convention for single variable
     Private routine called by nco_xtr_cf_add()
     http://cf-pcmdi.llnl.gov/documents/cf-conventions/1.1/cf-conventions.html#coordinate-system */ 

  char **cf_lst; /* [sng] 1D array of list elements */

  char att_nm[NC_MAX_NAME]; /* [sng] Attribute name */

  const char dlm_sng[]=" "; /* [sng] Delimiter string */

  int grp_id; /* [id] Group ID */
  int nbr_att; /* [nbr] Number of attributes */
  int nbr_cf; /* [nbr] Number of coordinates specified in "bounds" or "coordinates" attribute */
  int var_id; /* [id] Variable ID */

  assert(var_trv->nco_typ == nco_obj_typ_var);

  /* Obtain group ID from netCDF API using full group name */
  (void)nco_inq_grp_full_ncid(nc_id,var_trv->grp_nm_fll,&grp_id);

  /* Obtain variable ID */
  (void)nco_inq_varid(grp_id,var_trv->nm,&var_id);

  /* Find number of attributes */
  (void)nco_inq_varnatts(grp_id,var_id,&nbr_att);

  assert(nbr_att == var_trv->nbr_att);

  /* Loop attributes */
  for(int idx_att=0;idx_att<nbr_att;idx_att++){

    /* Get attribute name */
    (void)nco_inq_attname(grp_id,var_id,idx_att,att_nm);

    /* Is attribute part of CF convention? */
    if(!strcmp(att_nm,cf_nm)){
      char *att_val;
      long att_sz;
      nc_type att_typ;

      /* Yes, get list of specified attributes */
      (void)nco_inq_att(grp_id,var_id,att_nm,&att_typ,&att_sz);
      if(att_typ != NC_CHAR){
        (void)fprintf(stderr,"%s: WARNING \"%s\" attribute for variable %s is type %s, not %s. This violates CF convention for specifying additional attributes. Therefore will skip this attribute.\n",
          prg_nm_get(),att_nm,var_trv->nm_fll,nco_typ_sng(att_typ),nco_typ_sng(NC_CHAR));
        return;
      } /* end if */
      att_val=(char *)nco_malloc((att_sz+1L)*sizeof(char));
      if(att_sz > 0L) (void)nco_get_att(grp_id,var_id,att_nm,(void *)att_val,NC_CHAR);	  
      /* NUL-terminate attribute */
      att_val[att_sz]='\0';

      /* Split list into separate coordinate names
      Use nco_lst_prs_sgl_2D() not nco_lst_prs_2D() to avert TODO nco944 */
      cf_lst=nco_lst_prs_sgl_2D(att_val,dlm_sng,&nbr_cf);
      /* ...for each coordinate in CF convention attribute, i.e., "bounds" or "coordinate"... */
      for(int idx_cf=0;idx_cf<nbr_cf;idx_cf++){
        char *cf_lst_var=cf_lst[idx_cf];
        if(!cf_lst_var) continue;

        nco_bool flg_cf_fnd=False; /* [flg] Used to print an error message that CF variable was not found */

        /* Does CF-variable actually exist in input file, at least by its short name?. Find them all... */
        for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++){
          trv_sct trv=trv_tbl->lst[uidx];
          if(trv.nco_typ == nco_obj_typ_var && !strcmp(trv.nm,cf_lst_var)){

            /* Mark variable for extraction */
            trv_tbl->lst[uidx].flg_cf=True;
            trv_tbl->lst[uidx].flg_xtr=True;
            flg_cf_fnd=True;
          }
        } /* end loop over uidx */

        /* CF not found ? */
        if(flg_cf_fnd == False){     
          (void)fprintf(stderr,"%s: WARNING Variable %s, specified in \"%s\" attribute of variable %s, is not present in input file\n",
            prg_nm_get(),cf_lst[idx_cf],cf_nm,var_trv->nm_fll);
        } /* CF not found ? */
      } /* end loop over idx_cf */

      /* Free allocated memory */
      att_val=(char *)nco_free(att_val);
      cf_lst=nco_sng_lst_free(cf_lst,nbr_cf);

    } /* end strcmp() */
  } /* end loop over attributes */

  return;
} /* nco_xtr_cf_prv_add() */



nm_id_sct *                           /* O [sct] Extraction list */  
nco_trv_tbl_nm_id                     /* [fnc] Create extraction list of nm_id_sct from traversal table */
(const int nc_id,                     /* I [id] netCDF file ID */
 int * const xtr_nbr,                 /* I/O [nbr] Number of variables in extraction list */
 const trv_tbl_sct * const trv_tbl)   /* I [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Create extraction list of nm_id_sct from traversal table */

  nm_id_sct *xtr_lst; /* [sct] Extraction list */
  
  int nbr_tbl=0; 
  for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++){
    if(trv_tbl->lst[uidx].nco_typ == nco_obj_typ_var && trv_tbl->lst[uidx].flg_xtr){
      nbr_tbl++;
    } /* end flg == True */
  } /* end loop over uidx */

  xtr_lst=(nm_id_sct *)nco_malloc(nbr_tbl*sizeof(nm_id_sct));

  nbr_tbl=0;
  for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++){
    if(trv_tbl->lst[uidx].nco_typ == nco_obj_typ_var && trv_tbl->lst[uidx].flg_xtr){
      xtr_lst[nbr_tbl].var_nm_fll=(char *)strdup(trv_tbl->lst[uidx].nm_fll);
      xtr_lst[nbr_tbl].nm=(char *)strdup(trv_tbl->lst[uidx].nm);
      xtr_lst[nbr_tbl].grp_nm_fll=(char *)strdup(trv_tbl->lst[uidx].grp_nm_fll);
      /* 20130213: Necessary to allow MM3->MM4 and MM4->MM3 workarounds
	 Store in/out group IDs as determined in nco_xtr_dfn() 
	 In MM3/4 cases, either grp_in_id or grp_out_id are always root
	 Other is always root unless GPE is used */
      xtr_lst[nbr_tbl].grp_id_in=trv_tbl->lst[uidx].grp_id_in;
      xtr_lst[nbr_tbl].grp_id_out=trv_tbl->lst[uidx].grp_id_out;

      /* To deprecate: variable ID valid only in a netCDF3 context */
      int var_id;
      int grp_id;
      (void)nco_inq_grp_full_ncid(nc_id,trv_tbl->lst[uidx].grp_nm_fll,&grp_id);
      (void)nco_inq_varid(grp_id,trv_tbl->lst[uidx].nm,&var_id);
      xtr_lst[nbr_tbl].id=var_id;

      nbr_tbl++;
    } /* end flg == True */
  } /* end loop over uidx */

  *xtr_nbr=nbr_tbl;
  return xtr_lst;
} /* end nco_trv_tbl_nm_id() */

void
nco_xtr_crd_ass_add                   /* [fnc] Add to extraction list all coordinates associated with extracted variables */
(const int nc_id,                     /* I [ID] netCDF file ID */
 trv_tbl_sct * const trv_tbl)         /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Add to extraction list all coordinates associated with extracted variables */

  char dmn_nm_var[NC_MAX_NAME];    /* [sng] Dimension name for *variable* */ 

  int dmn_id_var[NC_MAX_DIMS]; /* [ID] Dimensions IDs array for variable */
  int grp_id;                  /* [ID] Group ID */
  int nbr_dmn_var;             /* [nbr] Number of dimensions associated with current matched variable */
  int var_id;                  /* [ID] Variable ID */

  long dmn_sz;                 /* [nbr] Dimension size */  

  for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++){
    trv_sct var_trv=trv_tbl->lst[uidx];
    if(var_trv.nco_typ == nco_obj_typ_var && var_trv.flg_xtr){

      /* Obtain group ID using full group name */
      (void)nco_inq_grp_full_ncid(nc_id,var_trv.grp_nm_fll,&grp_id);

      /* Obtain variable ID using group ID */
      (void)nco_inq_varid(grp_id,var_trv.nm,&var_id);

      /* Get number of dimensions for *variable* */
      (void)nco_inq_varndims(grp_id,var_id,&nbr_dmn_var);

      assert(nbr_dmn_var == var_trv.nbr_dmn);

      /* Get dimension IDs for variable */
      (void)nco_inq_vardimid(grp_id,var_id,dmn_id_var);

      /* Loop over dimensions of variable */
      for(int idx_var_dim=0;idx_var_dim<nbr_dmn_var;idx_var_dim++){

        /* Get dimension name */
        (void)nco_inq_dim(grp_id,dmn_id_var[idx_var_dim],dmn_nm_var,&dmn_sz);

        char dmn_nm_grp[NC_MAX_NAME];    /* [sng] Dimension name for *group*  */ 
        const char sls_chr='/'; /* [chr] Slash character */
        const char sls_sng[]="/"; /* [sng] Slash string */
        char *ptr_chr; /* [sng] Pointer to character '/' in full name */
        int psn_chr; /* [nbr] Position of character '/' in in full name */

        const int flg_prn=1;         /* [flg] Dimensions in all parent groups will also be retrieved */ 

        int dmn_id_grp[NC_MAX_DIMS]; /* [id] Dimensions IDs array */
        int nbr_dmn_grp;             /* [nbr] Number of dimensions for *group* */

        /* Obtain number of dimensions visible to group */
        (void)nco_inq(grp_id,&nbr_dmn_grp,NULL,NULL,NULL);

        /* Obtain dimension IDs */
        (void)nco_inq_dimids(grp_id,&nbr_dmn_grp,dmn_id_grp,flg_prn);

        /* List dimensions */
        for(int dmn_idx=0;dmn_idx<nbr_dmn_grp;dmn_idx++){

          /* Get dimension info */
          (void)nco_inq_dim(grp_id,dmn_id_grp[dmn_idx],dmn_nm_grp,&dmn_sz);

          /* Does dimension match requested variable name (i.e., is it a coordinate variable?) */ 
          if(!strcmp(dmn_nm_grp,dmn_nm_var)){
            char *dmn_nm_fll;

            /* Construct full (dimension/coordinate) name using the full group name where original variable resides */
            dmn_nm_fll=(char *)nco_malloc(strlen(var_trv.grp_nm_fll)+strlen(dmn_nm_grp)+2L);
            strcpy(dmn_nm_fll,var_trv.grp_nm_fll);
            if(strcmp(var_trv.grp_nm_fll,sls_sng)) strcat(dmn_nm_fll,sls_sng);
            strcat(dmn_nm_fll,dmn_nm_grp);

            /* Brute-force approach to find valid "dmn_nm_fll":
            Start at grp_nm_fll/var_nm and build all possible paths with var_nm. 
            Use case is /g5/g5g1/rz variable with /g5/rlev coordinate var. */

            /* Find last occurence of '/' */
            ptr_chr=strrchr(dmn_nm_fll,sls_chr);
            psn_chr=ptr_chr-dmn_nm_fll;
            while(ptr_chr){
              /* If variable is on list, mark it for extraction */
              if(trv_tbl_fnd_var_nm_fll(dmn_nm_fll,trv_tbl)){

                (void)trv_tbl_mrk_xtr(dmn_nm_fll,trv_tbl);

              } /* endif */
              dmn_nm_fll[psn_chr]='\0';
              ptr_chr=strrchr(dmn_nm_fll,sls_chr);
              if(ptr_chr){
                psn_chr=ptr_chr-dmn_nm_fll;
                dmn_nm_fll[psn_chr]='\0';
                /* Re-add variable name to shortened path */
                if(strcmp(var_trv.grp_nm_fll,sls_sng)) strcat(dmn_nm_fll,sls_sng);
                strcat(dmn_nm_fll,dmn_nm_grp);
                ptr_chr=strrchr(dmn_nm_fll,sls_chr);
                psn_chr=ptr_chr-dmn_nm_fll;
              } /* !ptr_chr */
            } /* end while */

            /* Free allocated */
            if(dmn_nm_fll) dmn_nm_fll=(char *)nco_free(dmn_nm_fll);

          } /* end strcmp() */
        } /* end loop over dmn_idx */


      } /* End loop over idx_var_dim: list dimensions for variable */
    } /* end nco_obj_typ_var */
  } /* end uidx  */
  return;
} /* end nco_xtr_crd_ass_cdf_add */

void
nco_get_prg_info(void)                 /* [fnc] Get program info */
{
  /* fxm: this routine is a kludge for Perl in nco_bm.pl and should be eliminated at first opportunity */
  int ret=10;
#ifndef HAVE_NETCDF4_H 
  ret=20;
#else /* HAVE_NETCDF4_H */
#ifdef ENABLE_NETCDF4 
  ret=30;
#else /* HAVE_NETCDF4_H */
  ret=40;
#endif /* ENABLE_NETCDF4 */
#endif /* HAVE_NETCDF4_H */

  exit(ret);
} /* end nco_get_prg_info() */

void
nco_prn_xtr_dfn /* [fnc] Print variable metadata */
(const int nc_id, /* I [id] netCDF file ID */
 const trv_tbl_sct * const trv_tbl) /* I [sct] GTT (Group Traversal Table) */
{ 
  for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++){
    trv_sct var_trv=trv_tbl->lst[uidx];
    if(var_trv.flg_xtr && var_trv.nco_typ == nco_obj_typ_var){

      /* Print full name of variable */
      if(var_trv.grp_dpt > 0) (void)fprintf(stdout,"%s\n",var_trv.nm_fll);

      /* Print variable metadata. NOTE: using file ID and object...all that is needed */ 
      (void)nco_prn_var_dfn(nc_id,&var_trv); 

      int grp_id; /* [id] Group ID */
      int var_id; /* [id] Variable ID */

      /* Obtain group ID using full group name */
      (void)nco_inq_grp_full_ncid(nc_id,var_trv.grp_nm_fll,&grp_id);

      /* Obtain variable ID using group ID */
      (void)nco_inq_varid(grp_id,var_trv.nm,&var_id);

      /* Print variable attributes */
      /* fxm pvn: rewrite with NC_ID and OBJ */
      (void)nco_prn_att(nc_id,grp_id,var_id);
    } /* end flg_xtr */
  } /* end uidx */

  return;
} /* end nco_prn_xtr_dfn() */

void 
xtr_lst_prn                            /* [fnc] Print name-ID structure list */
(nm_id_sct * const nm_id_lst,          /* I [sct] Name-ID structure list */
 const int nm_id_nbr)                  /* I [nbr] Number of name-ID structures in list */
{
  (void)fprintf(stdout,"%s: INFO List: %d extraction variables\n",prg_nm_get(),nm_id_nbr); 
  for(int idx=0;idx<nm_id_nbr;idx++){
    nm_id_sct nm_id=nm_id_lst[idx];
    (void)fprintf(stdout,"[%d] %s\n",idx,nm_id.var_nm_fll); 
  } 
}/* end xtr_lst_prn() */

void 
nco_nm_id_cmp                         /* [fnc] Compare 2 name-ID structure lists */
(nm_id_sct * const nm_id_lst1,        /* I [sct] Name-ID structure list */
 const int nm_id_nbr1,                /* I [nbr] Number of name-ID structures in list */
 nm_id_sct * const nm_id_lst2,        /* I [sct] Name-ID structure list */
 const int nm_id_nbr2,                /* I [nbr] Number of name-ID structures in list */
 const nco_bool SAME_ORDER)           /* I [flg] Both lists have the same order */
{
  int idx,jdx;
  assert(nm_id_nbr1 == nm_id_nbr2);
  if(SAME_ORDER){
    for(idx=0;idx<nm_id_nbr1;idx++){
      assert(strcmp(nm_id_lst1[idx].nm,nm_id_lst2[idx].nm) == 0);
      assert(strcmp(nm_id_lst1[idx].grp_nm_fll,nm_id_lst2[idx].grp_nm_fll) == 0);
      assert(strcmp(nm_id_lst1[idx].var_nm_fll,nm_id_lst2[idx].var_nm_fll) == 0);
    }
  }else{ /* SAME_ORDER */

    int nm_id_nbr=0;
    for(idx=0;idx<nm_id_nbr1;idx++){
      nm_id_sct nm_id_1=nm_id_lst1[idx];
      for(jdx=0;jdx<nm_id_nbr2;jdx++){
        nm_id_sct nm_id_2=nm_id_lst2[jdx];
        if(strcmp(nm_id_1.var_nm_fll,nm_id_2.var_nm_fll) == 0){
          nm_id_nbr++;
          assert(strcmp(nm_id_lst1[idx].nm,nm_id_lst2[jdx].nm) == 0);
          assert(strcmp(nm_id_lst1[idx].grp_nm_fll,nm_id_lst2[jdx].grp_nm_fll) == 0);
          assert(strcmp(nm_id_lst1[idx].var_nm_fll,nm_id_lst2[jdx].var_nm_fll) == 0);
        }
      }/* jdx */
    }/* idx */
    assert(nm_id_nbr == nm_id_nbr1);
  } /* SAME_ORDER */
} /* end nco_nm_id_cmp() */

void
nco_trv_tbl_chk                       /* [fnc] Validate trv_tbl_sct from a nm_id_sct input */
(const int nc_id,                     /* I [id] netCDF file ID */
 nm_id_sct * const xtr_lst,           /* I [sct] Extraction list  */
 const int xtr_nbr,                   /* I [nbr] Number of variables in extraction list */
 const trv_tbl_sct * const trv_tbl,   /* I [sct] GTT (Group Traversal Table) */
 const nco_bool NM_ID_SAME_ORDER)     /* I [flg] Both nm_id_sct have the same order */
{
  nm_id_sct *xtr_lst_chk=NULL;
  int xtr_nbr_chk;

  if(dbg_lvl_get() >= nco_dbg_dev){
    (void)xtr_lst_prn(xtr_lst,xtr_nbr);
    (void)trv_tbl_prn_xtr(trv_tbl,"nco_trv_tbl_chk()");
  }
  xtr_lst_chk=nco_trv_tbl_nm_id(nc_id,&xtr_nbr_chk,trv_tbl);
  (void)nco_nm_id_cmp(xtr_lst_chk,xtr_nbr_chk,xtr_lst,xtr_nbr,NM_ID_SAME_ORDER);
  if(xtr_lst_chk != NULL)xtr_lst_chk=nco_nm_id_lst_free(xtr_lst_chk,xtr_nbr_chk);
  return;
} /* end nco_trv_tbl_chk() */



void
nco_prn_var_val                       /* [fnc] Print variable data (called with PRN_VAR_DATA) */
(const int nc_id,                     /* I netCDF file ID */
 char * const dlm_sng,                /* I [sng] User-specified delimiter string, if any */
 const nco_bool FORTRAN_IDX_CNV,      /* I [flg] Hyperslab indices obey Fortran convention */
 const nco_bool MD5_DIGEST,           /* I [flg] Perform MD5 digests */
 const nco_bool PRN_DMN_UNITS,        /* I [flg] Print units attribute, if any */
 const nco_bool PRN_DMN_IDX_CRD_VAL,  /* I [flg] Print dimension/coordinate indices/values */
 const nco_bool PRN_DMN_VAR_NM,       /* I [flg] Print dimension/variable names */
 const nco_bool PRN_MSS_VAL_BLANK,    /* I [flg] Print missing values as blanks */
 const trv_tbl_sct * const trv_tbl)   /* I [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Print variable data (called with PRN_VAR_DATA) */

  /* Loop variables in table */
  for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){
    trv_sct var_trv=trv_tbl->lst[var_idx];
    if(var_trv.flg_xtr && var_trv.nco_typ == nco_obj_typ_var){

      /* Print full name of variable */
      if(!dlm_sng && var_trv.grp_dpt > 0) (void)fprintf(stdout,"%s\n",var_trv.nm_fll);

      /* Print variable values */
      (void)nco_msa_prn_var_val_trv(nc_id,dlm_sng,FORTRAN_IDX_CNV,MD5_DIGEST,PRN_DMN_UNITS,PRN_DMN_IDX_CRD_VAL,PRN_DMN_VAR_NM,PRN_MSS_VAL_BLANK,&var_trv);

    } /* End flg_xtr */
  } /* End Loop variables in table */

  return;
} /* end nco_prn_var_val() */

void
nco_xtr_dfn                          /* [fnc] Define extracted groups, variables, and attributes in output file */
(const int nc_id,                    /* I [ID] netCDF input file ID */
 const int nc_out_id,                /* I [ID] netCDF output file ID */
 int * const cnk_map_ptr,            /* I [enm] Chunking map */
 int * const cnk_plc_ptr,            /* I [enm] Chunking policy */
 const size_t cnk_sz_scl,            /* I [nbr] Chunk size scalar */
 CST_X_PTR_CST_PTR_CST_Y(cnk_sct,cnk), /* I [sct] Chunking information */
 const int cnk_nbr,                  /* I [nbr] Number of dimensions with user-specified chunking */
 const int dfl_lvl,                  /* I [enm] Deflate level [0..9] */
 const gpe_sct * const gpe,          /* I [sct] GPE structure */
 const nco_bool CPY_GRP_METADATA,    /* I [flg] Copy group metadata (attributes) */
 const nco_bool CPY_VAR_METADATA,    /* I [flg] Copy variable metadata (attributes) */
 const char * const rec_dmn_nm,      /* I [sng] Record dimension name */
 trv_tbl_sct * const trv_tbl)        /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Define groups, variables, and attributes in output file
     rec_dmn_nm, if any, is name requested for (netCDF3) sole record dimension */

  const char fnc_nm[]="nco_xtr_dfn()"; /* [sng] Function name */
  const char sls_sng[]="/"; /* [sng] Slash string */

  char *grp_out_fll; /* [sng] Group name */

  gpe_nm_sct *gpe_nm; /* [sct] GPE name duplicate check array */

  int fl_fmt; /* [enm] netCDF file format */
  int grp_id; /* [ID] Group ID in input file */
  int grp_out_id; /* [ID] Group ID in output file */ 
  int nbr_gpe_nm; /* [nbr] Number of GPE entries */
  int var_out_id; /* [ID] Variable ID in output file */

  nbr_gpe_nm=0;
  (void)nco_inq_format(nc_out_id,&fl_fmt);

  /* Isolate extra complexity of copying group metadata */
  if(CPY_GRP_METADATA){
    /* Block can be performed before or after writing variables
       Perhaps it should be turned into an explicit function call */
    
    /* Goal here is to annotate which groups will appear in output
       Need to know in order to efficiently copy their metadata
       Definition of flags in extraction table is operational
       Could create a new flag just for this
       Instead, we re-purpose the extraction flag, flg_xtr, for groups
       Could re-purpose flg_ncs too with same effect
       nco_xtr_mk() sets flg_xtr for groups, like variables, that match user-specified strings
       Later processing makes flg_xtr for groups unreliable
       For instance, the exclusion flag (-x) is ambiguous for groups
       Also identification of associated coordinates and auxiliary coordinates occurs after nco_xtr_mk()
       Associated and auxiliary coordinates may be in distant groups
       Hence no better place than nco_xtr_dfn() to finally identify ancestor groups */
    
    /* Set extraction flag for groups if ancestors of extracted variables.*/
    for(unsigned grp_idx=0;grp_idx<trv_tbl->nbr;grp_idx++){
      /* For each group ... */
      if(trv_tbl->lst[grp_idx].nco_typ == nco_obj_typ_grp){
        char *sbs_srt; /* [sng] Location of user-string match start in object path */
        char *grp_fll_sls=NULL; /* [sng] Full group name with slash appended */
        /* Initialize extraction flag to False and overwrite later iff ... */
        trv_tbl->lst[grp_idx].flg_xtr=False;
        if(!strcmp(trv_tbl->lst[grp_idx].grp_nm_fll,sls_sng)){
          /* Manually mark root group as extracted because matching algorithm below fails for root group 
          (it looks for "//" in variable names) */
          trv_tbl->lst[grp_idx].flg_xtr=True;
          continue;
        } /* endif root group */
        grp_fll_sls=(char *)strdup(trv_tbl->lst[grp_idx].grp_nm_fll);
        grp_fll_sls=(char *)nco_realloc(grp_fll_sls,(strlen(grp_fll_sls)+2L)*sizeof(char));
        strcat(grp_fll_sls,sls_sng);
        /* ... loop through ... */
        for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){
          /* ... all variables to be extracted ... */
          if(trv_tbl->lst[var_idx].nco_typ == nco_obj_typ_var && trv_tbl->lst[var_idx].flg_xtr){
            /* ... finds that full path to current group is contained in an extracted variable path ... */
            if((sbs_srt=strstr(trv_tbl->lst[var_idx].nm_fll,grp_fll_sls))){
              /* ... and _begins_ a full group path of that variable ... */
              if(sbs_srt == trv_tbl->lst[var_idx].nm_fll){
                /* ... and mark _only_ those groups for extraction... */
                trv_tbl->lst[grp_idx].flg_xtr=True;
                continue;
              } /* endif */
            } /* endif full group path */
          } /* endif extracted variable */
        } /* end loop over var_idx */
        if(grp_fll_sls) grp_fll_sls=(char *)nco_free(grp_fll_sls);
      } /* endif group */
    } /* end loop over grp_idx */

    for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++){
      trv_sct grp_trv=trv_tbl->lst[uidx];

      /* If object is group ancestor of extracted variable */
      if(grp_trv.nco_typ == nco_obj_typ_grp && grp_trv.flg_xtr){

        /* Obtain group ID from netCDF API using full group name */
        (void)nco_inq_grp_full_ncid(nc_id,grp_trv.grp_nm_fll,&grp_id);

        /* Edit group name for output */
        if(gpe) grp_out_fll=nco_gpe_evl(gpe,grp_trv.grp_nm_fll); else grp_out_fll=(char *)strdup(grp_trv.grp_nm_fll);

        /* If output group does not exist, create it */
        if(nco_inq_grp_full_ncid_flg(nc_out_id,grp_out_fll,&grp_out_id)) {
          nco_def_grp_full(nc_out_id,grp_out_fll,&grp_out_id);
        } /* Create group */

        /* Copy group attributes */
        if(grp_trv.nbr_att) (void)nco_att_cpy(grp_id,grp_out_id,NC_GLOBAL,NC_GLOBAL,(nco_bool)True);

        /* Memory management after current extracted group */
        if(grp_out_fll) grp_out_fll=(char *)nco_free(grp_out_fll);

      } /* end if group and flg_xtr */
    } /* end loop to define group attributes */

  } /* !CPY_GRP_METADATA */

  /* Define variables */
  for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++){
    trv_sct var_trv=trv_tbl->lst[uidx];

    int nbr_dmn; /* [nbr] Number of dimensions in input group */

    /* If object is an extracted variable... */
    if(var_trv.nco_typ == nco_obj_typ_var && var_trv.flg_xtr){

      /* Obtain group ID using full group name */
      (void)nco_inq_grp_full_ncid(nc_id,var_trv.grp_nm_fll,&grp_id);

      /* Obtain info for group */
      (void)nco_inq(grp_id,&nbr_dmn,(int *)NULL,(int *)NULL,NULL);

      /* Edit group name for output */
      if(gpe) grp_out_fll=nco_gpe_evl(gpe,var_trv.grp_nm_fll); else grp_out_fll=(char *)strdup(var_trv.grp_nm_fll);

      /* If output group does not exist, create it */
      if(nco_inq_grp_full_ncid_flg(nc_out_id,grp_out_fll,&grp_out_id)){
        nco_def_grp_full(nc_out_id,grp_out_fll,&grp_out_id);
      } /* Create group */

      /* Detect duplicate GPE names in advance, then exit with helpful error */
      if(gpe){
        char *gpe_var_nm_fll=NULL; 

        /* Construct absolute GPE variable path */
        gpe_var_nm_fll=(char*)nco_malloc(strlen(grp_out_fll)+strlen(var_trv.nm)+2L);
        strcpy(gpe_var_nm_fll,grp_out_fll);
        /* If not root group, concatenate separator */
        if(strcmp(grp_out_fll,sls_sng)) strcat(gpe_var_nm_fll,sls_sng);
        strcat(gpe_var_nm_fll,var_trv.nm);

        /* GPE name is not already on list, put it there */
        if(nbr_gpe_nm == 0){
          gpe_nm=(gpe_nm_sct *)nco_malloc((nbr_gpe_nm+1)*sizeof(gpe_nm_sct)); 
          gpe_nm[nbr_gpe_nm].var_nm_fll=strdup(gpe_var_nm_fll);
          nbr_gpe_nm++;
        }else{
          /* Put GPE on list only if not already there */
          for(int idx_gpe=0;idx_gpe<nbr_gpe_nm;idx_gpe++){
            if(!strcmp(gpe_var_nm_fll,gpe_nm[idx_gpe].var_nm_fll)){
              (void)fprintf(stdout,"%s: ERROR %s reports variable %s already defined. HINT: Removing groups to flatten files can lead to over-determined situations where a single object name (e.g., a variable name) must refer to multiple objects in the same output group. The user's intent is ambiguous so instead of arbitrarily picking which (e.g., the last) variable of that name to place in the output file, NCO simply fails. User should re-try command after ensuring multiple objects of the same name will not be placed in the same group.\n",prg_nm_get(),fnc_nm,gpe_var_nm_fll);
              for(int idx=0;idx<nbr_gpe_nm;idx++) gpe_nm[idx].var_nm_fll=(char *)nco_free(gpe_nm[idx].var_nm_fll);
              nco_exit(EXIT_FAILURE);
            } /* strcmp() */
          } /* end loop over gpe_nm */
          gpe_nm=(gpe_nm_sct *)nco_realloc((void *)gpe_nm,(nbr_gpe_nm+1)*sizeof(gpe_nm_sct));
          gpe_nm[nbr_gpe_nm].var_nm_fll=strdup(gpe_var_nm_fll);
          nbr_gpe_nm++;
        } /* nbr_gpe_nm */

        /* Free full path name */
        if(gpe_var_nm_fll) gpe_var_nm_fll=(char *)nco_free(gpe_var_nm_fll);
      } /* !GPE */


      if(dbg_lvl_get() >= nco_dbg_dev){
        (void)fprintf(stdout,"%s: INFO %s defining variable <%s> from ",prg_nm_get(),fnc_nm,var_trv.nm_fll);        
        (void)nco_prt_grp_nm_fll(grp_id);
        (void)fprintf(stdout," to ");   
        (void)nco_prt_grp_nm_fll(grp_out_id);
        (void)fprintf(stdout,"\n");
      } /* endif dbg */

      /* Define variable in output file */
      var_out_id=nco_cpy_var_dfn(nc_id,nc_out_id,grp_id,grp_out_id,dfl_lvl,gpe,rec_dmn_nm,&var_trv,trv_tbl);

      /* Set chunksize parameters */
      if(fl_fmt == NC_FORMAT_NETCDF4 || fl_fmt == NC_FORMAT_NETCDF4_CLASSIC) (void)nco_cnk_sz_set_trv(grp_out_id,cnk_map_ptr,cnk_plc_ptr,cnk_sz_scl,cnk,cnk_nbr,&var_trv);

      /* Copy variable's attributes */
      if(CPY_VAR_METADATA){
        int var_id;
        (void)nco_inq_varid(grp_id,var_trv.nm,&var_id);
        (void)nco_att_cpy(grp_id,grp_out_id,var_id,var_out_id,(nco_bool)True);
      } /* !CPY_VAR_METADATA */

      /* Memory management after current extracted variable */
      if(grp_out_fll) grp_out_fll=(char *)nco_free(grp_out_fll);

      /* Store input and output group IDs for use by nco_xtr_wrt() */
      trv_tbl->lst[uidx].grp_id_in=grp_id;
      trv_tbl->lst[uidx].grp_id_out=grp_out_id;

    } /* end if variable and flg_xtr */

  } /* end loop over uidx */

  /* Memory management for GPE names */
  for(int idx=0;idx<nbr_gpe_nm;idx++) gpe_nm[idx].var_nm_fll=(char *)nco_free(gpe_nm[idx].var_nm_fll);

  /* Print extraction list in developer mode */
  if(dbg_lvl_get() == 13) (void)trv_tbl_prn_xtr(trv_tbl,fnc_nm);

} /* end nco_xtr_dfn() */

void
nco_xtr_wrt                           /* [fnc] Write extracted data to output file */
(const int nc_in_id,                  /* I [ID] netCDF input file ID */
 const int nc_out_id,                 /* I [ID] netCDF output file ID */
 FILE * const fp_bnr,                 /* I [fl] Unformatted binary output file handle */
 const nco_bool MD5_DIGEST,           /* I [flg] Perform MD5 digests */
 const nco_bool HAVE_LIMITS,          /* I [flg] Dimension limits exist ( For convenience, ideally... not needed ) */
 const trv_tbl_sct * const trv_tbl)   /* I [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Write extracted variables to output file */

  const char fnc_nm[]="nco_xtr_wrt()"; /* [sng] Function name */

  int fl_out_fmt; /* [enm] File format */

  nco_bool USE_MM3_WORKAROUND=False; /* [flg] Faster copy on Multi-record Multi-variable netCDF3 files */

  (void)nco_inq_format(nc_out_id,&fl_out_fmt);

  /* 20120309 Special case to improve copy speed on large blocksize filesystems (MM3s) */
  USE_MM3_WORKAROUND=nco_use_mm3_workaround(nc_in_id,fl_out_fmt);
  if(HAVE_LIMITS) USE_MM3_WORKAROUND=False; 

  if(USE_MM3_WORKAROUND){  

    int fix_nbr; /* [nbr] Number of fixed-length variables */
    int rec_nbr; /* [nbr] Number of record variables */
    int xtr_nbr; /* [nbr] Number of extracted variables */
    int var_idx; /* [idx] */

    nm_id_sct **fix_lst=NULL; /* [sct] Fixed-length variables to be extracted */
    nm_id_sct **rec_lst=NULL; /* [sct] Record variables to be extracted */
    nm_id_sct *xtr_lst=NULL; /* [sct] Variables to be extracted */

    if(dbg_lvl_get() >= nco_dbg_std) (void)fprintf(stderr,"%s: INFO Using MM3-workaround to hasten copying of record variables\n",prg_nm_get());

    /* Convert extraction list from traversal table to nm_id_sct format to re-use old code */
    xtr_lst=nco_trv_tbl_nm_id(nc_in_id,&xtr_nbr,trv_tbl);

    /* Split list into fixed-length and record variables */
    (void)nco_var_lst_fix_rec_dvd(nc_in_id,xtr_lst,xtr_nbr,&fix_lst,&fix_nbr,&rec_lst,&rec_nbr);

    /* Copy fixed-length data variable-by-variable */
    for(var_idx=0;var_idx<fix_nbr;var_idx++){
      if(dbg_lvl_get() >= nco_dbg_var && !fp_bnr) (void)fprintf(stderr,"%s, ",fix_lst[var_idx]->nm);
      if(dbg_lvl_get() >= nco_dbg_var) (void)fflush(stderr);
      (void)nco_cpy_var_val(fix_lst[var_idx]->grp_id_in,fix_lst[var_idx]->grp_id_out,fp_bnr,MD5_DIGEST,fix_lst[var_idx]->nm);
    } /* end loop over var_idx */

    /* Copy record data record-by-record */
    (void)nco_cpy_rec_var_val(nc_in_id,fp_bnr,MD5_DIGEST,rec_lst,rec_nbr);

    /* Extraction lists no longer needed */
    if(fix_lst) fix_lst=(nm_id_sct **)nco_free(fix_lst);
    if(rec_lst) rec_lst=(nm_id_sct **)nco_free(rec_lst);
    if(xtr_lst) xtr_lst=nco_nm_id_lst_free(xtr_lst,xtr_nbr);

  }else{ /* !USE_MM3_WORKAROUND */

    for(unsigned uidx=0;uidx<trv_tbl->nbr;uidx++){
      trv_sct trv=trv_tbl->lst[uidx];

      /* If object is an extracted variable... */ 
      if(trv.nco_typ == nco_obj_typ_var && trv.flg_xtr){

        if(dbg_lvl_get() >= nco_dbg_dev){
          (void)fprintf(stdout,"%s: INFO %s writing variable <%s> from ",prg_nm_get(),fnc_nm,trv.nm_fll);        
          (void)nco_prt_grp_nm_fll(trv.grp_id_in);
          (void)fprintf(stdout," to ");   
          (void)nco_prt_grp_nm_fll(trv.grp_id_out);
          (void)fprintf(stdout,"\n");
        } /* endif dbg */


        if(HAVE_LIMITS) (void)nco_cpy_var_val_mlt_lmt_trv(trv.grp_id_in,trv.grp_id_out,fp_bnr,MD5_DIGEST,&trv); 
        else (void)nco_cpy_var_val(trv.grp_id_in,trv.grp_id_out,fp_bnr,MD5_DIGEST,trv.nm);
      } /* endif */

    } /* end loop over uidx */
  } /* !USE_MM3_WORKAROUND */


  /* Print extraction list in developer mode */
  if(dbg_lvl_get() == 13) (void)trv_tbl_prn_xtr(trv_tbl,fnc_nm);

} /* end nco_xtr_wrt() */

void                          
nco_prt_dmn /* [fnc] Print dimensions for a group  */
(const int nc_id, /* I [ID] File ID */
 const char * const grp_nm_fll) /* I [sng] Full name of group */
{
  char dmn_nm[NC_MAX_NAME]; /* [sng] Dimension name */ 

  const int flg_prn=0; /* [flg] Retrieve all dimensions in parent groups */        

  int dmn_ids[NC_MAX_DIMS]; /* [nbr] Dimensions IDs array */
  int dmn_ids_ult[NC_MAX_DIMS]; /* [nbr] Unlimited dimensions IDs array */
  int grp_id; /* [ID]  Group ID */
  int nbr_att; /* [nbr] Number of attributes */
  int nbr_dmn; /* [nbr] Number of dimensions */
  int nbr_dmn_ult; /* [nbr] Number of unlimited dimensions */
  int nbr_var; /* [nbr] Number of variables */

  long dmn_sz; /* [nbr] Dimension size */

  /* Obtain group ID from netCDF API using full group name */
  (void)nco_inq_grp_full_ncid(nc_id,grp_nm_fll,&grp_id);

  /* Obtain unlimited dimensions for group */
  (void)nco_inq_unlimdims(grp_id,&nbr_dmn_ult,dmn_ids_ult);

  /* Obtain number of dimensions for group */
  (void)nco_inq(grp_id,&nbr_dmn,&nbr_var,&nbr_att,NULL);

  /* Obtain dimensions IDs for group */
  (void)nco_inq_dimids(grp_id,&nbr_dmn,dmn_ids,flg_prn);

  /* List dimensions using obtained group ID */
  for(int dnm_idx=0;dnm_idx<nbr_dmn;dnm_idx++){
    nco_bool is_rec_dim=False;
    (void)nco_inq_dim(grp_id,dmn_ids[dnm_idx],dmn_nm,&dmn_sz);

    /* Check if dimension is unlimited (record dimension) */
    for(int dnm_ult_idx=0;dnm_ult_idx<nbr_dmn_ult;dnm_ult_idx++){ 
      if(dmn_ids[dnm_idx] == dmn_ids_ult[dnm_ult_idx]){ 
        is_rec_dim=True;
        (void)fprintf(stdout," #%d record dimension: '%s'(%li)\n",dmn_ids[dnm_idx],dmn_nm,dmn_sz);
      } /* end if */
    } /* end dnm_ult_idx dimensions */

    /* An unlimited ID was not matched, so dimension is a plain vanilla dimension */
    if(!is_rec_dim) (void)fprintf(stdout," #%d dimension: '%s'(%li)\n",dmn_ids[dnm_idx],dmn_nm,dmn_sz);

  } /* end dnm_idx dimensions */
} /* end nco_prt_dmn() */


dmn_trv_sct *                         /* O [sct] GTT dimension structure (stored in *groups*) */
nco_dmn_trv_sct                       /* [fnc] Return unique dimension object from unique ID */
(const int dmn_id,                    /* I [id] Unique dimension ID */
 const trv_tbl_sct * const trv_tbl)   /* I [sct] GTT (Group Traversal Table) */
{

  /* Search table dimension list */
  for(unsigned int dmn_lst_idx=0;dmn_lst_idx<trv_tbl->nbr_dmn;dmn_lst_idx++){

    /* Compare IDs */
    if (dmn_id == trv_tbl->lst_dmn[dmn_lst_idx].dmn_id){

      /* Return object  */
      return &trv_tbl->lst_dmn[dmn_lst_idx];

    } /* Compare IDs */
  } /* Search table dimension list */

  return NULL;

} /* nco_dmn_trv_sct() */


char *                                /* O [id] Unique dimension full name */
nco_dmn_fll_nm_id                     /* [fnc] Return unique dimension full name from unique ID  */
(const int dmn_id,                    /* I [id] Unique dimension ID */
 const trv_tbl_sct * const trv_tbl)   /* I [sct] GTT (Group Traversal Table) */
{

  /* Search table dimension list */
  for(unsigned int dmn_lst_idx=0;dmn_lst_idx<trv_tbl->nbr_dmn;dmn_lst_idx++){

    /* Compare IDs */
    if (dmn_id == trv_tbl->lst_dmn[dmn_lst_idx].dmn_id){

      /* Return object  */
      return trv_tbl->lst_dmn[dmn_lst_idx].nm_fll;

    } /* Compare IDs */
  } /* Search table dimension list */

  return NULL;

} /* nco_dmn_fll_nm_id() */

void                          
nco_bld_dmn_ids_trv                   /* [fnc] Build dimension info for all variables */
(const int nc_id,                     /* I [ID] netCDF file ID */
 trv_tbl_sct * const trv_tbl)         /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Purpose: a netCDF4 variable can have its dimensions located anywhere below *in the group path*
  Construction of this list *must* be done after traversal table is build in nco_grp_itr(),
  where we know the full picture of the file tree
  Compare unique dimension IDs from variables with unique dimension IDs from groups 
  */

  const char fnc_nm[]="nco_bld_dmn_ids_trv()"; /* [sng] Function name  */

  /* Loop objects  */
  if(dbg_lvl_get() == 14)(void)fprintf(stdout,"%s: INFO %s reports variable dimensions\n",prg_nm_get(),fnc_nm);
  for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){

    /* Filter variables  */
    if(trv_tbl->lst[var_idx].nco_typ == nco_obj_typ_var){
      trv_sct var_trv=trv_tbl->lst[var_idx];   

      if(dbg_lvl_get() == 14){
        (void)fprintf(stdout,"%s:",var_trv.nm_fll); 
        (void)fprintf(stdout," %d dimensions:\n",var_trv.nbr_dmn);
      }

      /* Full dimension names for each variable */
      for(int dmn_idx_var=0;dmn_idx_var<var_trv.nbr_dmn;dmn_idx_var++){

        int var_dmn_id=var_trv.var_dmn[dmn_idx_var].dmn_id;

        /* Get unique dimension object from unique dimension ID */
        dmn_trv_sct *dmn_trv=nco_dmn_trv_sct(var_dmn_id,trv_tbl);

        if(dbg_lvl_get() == 14){
          (void)fprintf(stdout,"[%d]%s#%d ",dmn_idx_var,var_trv.var_dmn[dmn_idx_var].dmn_nm,var_dmn_id);    
          (void)fprintf(stdout,"<%s>\n",dmn_trv->nm_fll);
        }
        if (strcmp(var_trv.var_dmn[dmn_idx_var].dmn_nm,dmn_trv->nm) != 0){


        /*      
        Test case generates duplicated dimension IDs in netCDF file

        ncks -O  -v two_dmn_rec_var in_grp.nc out.nc

        nco_cpy_var_dfn() defines new dimesions for the file, as

        ncks: INFO nco_cpy_var_dfn() defining dimensions
        ID=0 index [0]:</time> 
        ID=1 index [1]:</lev> 
        ID=2 index [0]:</g8/lev> 
        ID=3 index [1]:</g8/vrt_nbr> 
        ID=4 index [1]:</vrt_nbr> 

        but the resulting file, when read, has the following IDs

        dimensions:
        #0,time = UNLIMITED ; // (10 currently)
        #1,lev = 3 ;
        #4,vrt_nbr = 2 ;

        group: g8 {
        dimensions:
        #0,lev = 3 ;
        #1,vrt_nbr = 2 ;

        From: "Unidata netCDF Support" <support-netcdf@unidata.ucar.edu>
        To: <pvicente@uci.edu>
        Sent: Tuesday, March 12, 2013 5:02 AM
        Subject: [netCDF #SHH-257980]: Re: [netcdfgroup] Dimensions IDs

        > Your Ticket has been received, and a Unidata staff member will review it and reply accordingly. Listed below are details of this new Ticket. Please make sure the Ticket ID remains in the Subject: line on all correspondence related to this Ticket.
        > 
        >    Ticket ID: SHH-257980
        >    Subject: Re: [netcdfgroup] Dimensions IDs
        >    Department: Support netCDF
        >    Priority: Normal
        >    Status: Open

        */

          (void)fprintf(stdout,"%s: ERROR netCDF file with duplicate dimension IDs detected. Such files should not exist yet can be created by NCO (and other software) due to an apparent bug in the HDF or netCDF4 libraries. Unidata bug ticket is SHH-257980 filed 20130312.\n",prg_nm_get());
          (void)nco_prt_trv_tbl(nc_id,trv_tbl);
          nco_exit(EXIT_FAILURE);
        }


        /* Store full dimension name  */
        trv_tbl->lst[var_idx].var_dmn[dmn_idx_var].dmn_nm_fll=strdup(dmn_trv->nm_fll);

        /* Store full group name where dimension is located. NOTE: using member "grp_nm_fll" of dimension  */
        trv_tbl->lst[var_idx].var_dmn[dmn_idx_var].grp_nm_fll=strdup(dmn_trv->grp_nm_fll);

      }

    } /* Filter variables  */
  } /* Variables */

} /* end nco_blb_dmn_ids_trv() */




int /* [rcd] Return code */
nco_grp_itr /* [fnc] Populate traversal table by examining, recursively, subgroups of parent */
(const int grp_id, /* I [ID] Group ID */
 char * const grp_nm_fll, /* I [sng] Absolute group name (path) */
 trv_tbl_sct * const trv_tbl) /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Populate traversal table by examining, recursively, subgroups of parent */

  const char fnc_nm[]="nco_grp_itr()"; /* [sng] Function name */

  const char sls_sng[]="/"; /* [sng] Slash string */

  char grp_nm[NC_MAX_NAME+1]; /* [sng] Group name */
  char var_nm[NC_MAX_NAME+1]; /* [sng] Variable name */ 
  char dmn_nm[NC_MAX_NAME+1]; /* [sng] Dimension name */ 
  char rec_nm[NC_MAX_NAME+1]; /* [sng] Record dimension name */ 

  char *var_nm_fll;           /* [sng] Full path for variable */
  char *dmn_nm_fll;           /* [sng] Full path for dimension */
  char *sls_psn; /* [sng] Current position of group path search */

  const int flg_prn=0; /* [flg] All the dimensions in all parent groups will also be retrieved */    

  int dmn_ids_grp[NC_MAX_DIMS]; /* [ID]  Dimension IDs array for group */ 
  int dmn_ids_grp_ult[NC_MAX_DIMS];/* [ID] Unlimited (record) dimensions IDs array for group */
  int dmn_id_var[NC_MAX_DIMS]; /* [ID] Dimensions IDs array for variable */

  int *grp_ids; /* [ID] Sub-group IDs array */  

  int grp_dpt=0; /* [nbr] Depth of group (root = 0) */
  int nbr_att; /* [nbr] Number of attributes */
  int nbr_dmn_grp; /* [nbr] Number of dimensions for group */
  int nbr_dmn_var; /* [nbr] Number of dimensions for variable */
  int nbr_grp; /* [nbr] Number of sub-groups in this group */
  int nbr_rec; /* [nbr] Number of record dimensions in this group */
  int nbr_var; /* [nbr] Number of variables */
  int rcd=NC_NOERR; /* [rcd] Return code */

  long dmn_sz;                /* [nbr] Dimension size */ 
  long rec_sz;                /* [nbr] Record dimension size */ 

  nc_type var_typ; /* O [enm] NetCDF type */

  nco_obj_typ obj_typ; /* [enm] Object type (group or variable) */

  /* Get all information for this group */

  /* Get group name */
  rcd+=nco_inq_grpname(grp_id,grp_nm);

  /* Get number of sub-groups */
  rcd+=nco_inq_grps(grp_id,&nbr_grp,(int *)NULL);

  /* Obtain number of dimensions/variable/attributes for group; NB: ignore record dimension ID */
  rcd+=nco_inq(grp_id,&nbr_dmn_grp,&nbr_var,&nbr_att,(int *)NULL);

  /* Obtain dimensions IDs for group */
  rcd+=nco_inq_dimids(grp_id,&nbr_dmn_grp,dmn_ids_grp,flg_prn);

  /* Obtain unlimited dimensions for group */
  rcd+=nco_inq_unlimdims(grp_id,&nbr_rec,dmn_ids_grp_ult);

  /* Compute group depth */
  sls_psn=grp_nm_fll;
  if(!strcmp(grp_nm_fll,sls_sng)) grp_dpt=0; else grp_dpt=1;
  while((sls_psn=strchr(sls_psn+1,'/'))) grp_dpt++;
  if(dbg_lvl_get() == nco_dbg_crr) (void)fprintf(stderr,"%s: INFO Group %s is at level %d\n",prg_nm_get(),grp_nm_fll,grp_dpt);

  /* Keep the old table objects size for insertion */
  unsigned int idx;
  idx=trv_tbl->nbr;

  /* Add one more element to GTT (nco_realloc nicely handles first time/not first time insertions) */
  trv_tbl->nbr++;
  trv_tbl->lst=(trv_sct *)nco_realloc(trv_tbl->lst,trv_tbl->nbr*sizeof(trv_sct));

  /* Add this element (a group) to table */

  trv_tbl->lst[idx].nco_typ=nco_obj_typ_grp;          /* [enm] netCDF4 object type: group or variable */

  strcpy(trv_tbl->lst[idx].nm,grp_nm);            /* [sng] Relative name (i.e., variable name or last component of path name for groups) */
  trv_tbl->lst[idx].nm_lng=strlen(grp_nm);        /* [sng] Length of short name */
  trv_tbl->lst[idx].grp_nm_fll=strdup(grp_nm_fll);/* [sng] Full group name (for groups, same as nm_fll) */
  trv_tbl->lst[idx].nm_fll=strdup(grp_nm_fll);    /* [sng] Fully qualified name (path) */
  trv_tbl->lst[idx].nm_fll_lng=strlen(grp_nm_fll);/* [sng] Length of full name */

  trv_tbl->lst[idx].flg_cf=False;                 /* [flg] Object matches CF-metadata extraction criteria */
  trv_tbl->lst[idx].flg_crd=False;                /* [flg] Object matches coordinate extraction criteria */
  trv_tbl->lst[idx].flg_dfl=False;                /* [flg] Object meets default subsetting criteria */
  trv_tbl->lst[idx].flg_gcv=False;                /* [flg] Group contains matched variable */
  trv_tbl->lst[idx].flg_mch=False;                /* [flg] Object matches user-specified strings */
  trv_tbl->lst[idx].flg_ncs=False;                /* [flg] Group is ancestor of specified group or variable */
  trv_tbl->lst[idx].flg_nsx=False;                /* [flg] Object matches intersection criteria */
  trv_tbl->lst[idx].flg_rcr=False;                /* [flg] Extract group recursively */
  trv_tbl->lst[idx].flg_unn=False;                /* [flg] Object matches union criteria */
  trv_tbl->lst[idx].flg_vfp=False;                /* [flg] Variable matches full path specification */
  trv_tbl->lst[idx].flg_vsg=False;                /* [flg] Variable selected because group matches */
  trv_tbl->lst[idx].flg_xcl=False;                /* [flg] Object matches exclusion criteria */
  trv_tbl->lst[idx].flg_xtr=False;                /* [flg] Extract object */

  trv_tbl->lst[idx].grp_dpt=grp_dpt;              /* [nbr] Depth of group (root = 0) */
  trv_tbl->lst[idx].grp_id_in=nco_obj_typ_err;    /* [id] Group ID in input file */
  trv_tbl->lst[idx].grp_id_out=nco_obj_typ_err;   /* [id] Group ID in output file */

  trv_tbl->lst[idx].nbr_dmn=nbr_dmn_grp;          /* [nbr] Number of dimensions */
  trv_tbl->lst[idx].nbr_att=nbr_att;              /* [nbr] Number of attributes */
  trv_tbl->lst[idx].nbr_grp=nbr_grp;              /* [nbr] Number of sub-groups (for group) */
  trv_tbl->lst[idx].nbr_rec=nbr_rec;              /* [nbr] Number of record dimensions */
  trv_tbl->lst[idx].nbr_var=nbr_var;              /* [nbr] Number of variables (for group) */

  trv_tbl->lst[idx].is_crd_var=nco_obj_typ_err;   /* [flg] (For variables only) Is this a coordinate variable? (unique dimension exists in scope) */
  trv_tbl->lst[idx].is_rec_var=nco_obj_typ_err;   /* [flg] (For variables only) Is a record variable? (is_crd_var must be True) */
  trv_tbl->lst[idx].var_typ=(nc_type)nco_obj_typ_err;      /* [enm] (For variables only) NetCDF type  */  


  /* Variable dimensions  */
  for(int dmn_idx_var=0;dmn_idx_var<NC_MAX_DIMS;dmn_idx_var++){
    trv_tbl->lst[idx].var_dmn[dmn_idx_var].dmn_nm_fll=NULL;
    trv_tbl->lst[idx].var_dmn[dmn_idx_var].dmn_nm=NULL;
    trv_tbl->lst[idx].var_dmn[dmn_idx_var].grp_nm_fll=NULL;
    trv_tbl->lst[idx].var_dmn[dmn_idx_var].is_crd_var=nco_obj_typ_err;
    trv_tbl->lst[idx].var_dmn[dmn_idx_var].crd=NULL;
    trv_tbl->lst[idx].var_dmn[dmn_idx_var].ncd=NULL;
    trv_tbl->lst[idx].var_dmn[dmn_idx_var].dmn_id=nco_obj_typ_err;
  }
  

  /* Iterate variables for this group */
  for(int var_idx=0;var_idx<nbr_var;var_idx++){

    /* Get type of variable and number of dimensions */
    rcd+=nco_inq_var(grp_id,var_idx,var_nm,&var_typ,&nbr_dmn_var,(int *)NULL,&nbr_att);

    /* Get dimension IDs for variable */
    (void)nco_inq_vardimid(grp_id,var_idx,dmn_id_var);

    /* Allocate path buffer and include space for trailing NUL */ 
    var_nm_fll=(char *)nco_malloc(strlen(grp_nm_fll)+strlen(var_nm)+2L);

    /* Initialize path with current absolute group path */
    strcpy(var_nm_fll,grp_nm_fll);

    /* If not root group, concatenate separator */
    if(strcmp(grp_nm_fll,sls_sng)) strcat(var_nm_fll,sls_sng);

    /* Concatenate variable to absolute group path */
    strcat(var_nm_fll,var_nm);

    if(var_typ <= NC_MAX_ATOMIC_TYPE){
      obj_typ=nco_obj_typ_var;
    }else{ /* > NC_MAX_ATOMIC_TYPE */
      obj_typ=nco_obj_typ_nonatomic_var;
      if(dbg_lvl_get() >= nco_dbg_var) (void)fprintf(stderr,"%s: WARNING NCO only supports netCDF4 atomic-type variables. Variable %s is type %d = %s, and will be ignored in subsequent processing.\n",prg_nm_get(),var_nm_fll,var_typ,nco_typ_sng(var_typ));
    } /* > NC_MAX_ATOMIC_TYPE */


    /* Keep the old table objects size for insertion */
    idx=trv_tbl->nbr;

    /* Add one more element to GTT (nco_realloc nicely handles first time/not first time insertions) */
    trv_tbl->nbr++;
    trv_tbl->lst=(trv_sct *)nco_realloc(trv_tbl->lst,trv_tbl->nbr*sizeof(trv_sct));

    /* Add this element, a variable to table. NOTE: nbr_var, nbr_grp, flg_rcr not valid here */

    trv_tbl->lst[idx].nco_typ=obj_typ;

    strcpy(trv_tbl->lst[idx].nm,var_nm);
    trv_tbl->lst[idx].nm_lng=strlen(var_nm);
    trv_tbl->lst[idx].grp_nm_fll=strdup(grp_nm_fll);
    trv_tbl->lst[idx].nm_fll=strdup(var_nm_fll);
    trv_tbl->lst[idx].nm_fll_lng=strlen(var_nm_fll);  

    trv_tbl->lst[idx].flg_cf=False; 
    trv_tbl->lst[idx].flg_crd=False; 
    trv_tbl->lst[idx].flg_dfl=False; 
    trv_tbl->lst[idx].flg_gcv=False; 
    trv_tbl->lst[idx].flg_mch=False; 
    trv_tbl->lst[idx].flg_ncs=False; 
    trv_tbl->lst[idx].flg_nsx=False; 
    trv_tbl->lst[idx].flg_rcr=False; 
    trv_tbl->lst[idx].flg_unn=False; 
    trv_tbl->lst[idx].flg_vfp=False; 
    trv_tbl->lst[idx].flg_vsg=False; 
    trv_tbl->lst[idx].flg_xcl=False; 
    trv_tbl->lst[idx].flg_xtr=False; 

    trv_tbl->lst[idx].grp_dpt=grp_dpt; 
    trv_tbl->lst[idx].grp_id_in=nco_obj_typ_err; 
    trv_tbl->lst[idx].grp_id_out=nco_obj_typ_err; 

    trv_tbl->lst[idx].nbr_att=nbr_att;
    trv_tbl->lst[idx].nbr_dmn=nbr_dmn_var;
    trv_tbl->lst[idx].nbr_grp=nco_obj_typ_err;
    trv_tbl->lst[idx].nbr_rec=nbr_rec; /* NB: broken fxm should be record dimensions used by this variable */
    trv_tbl->lst[idx].nbr_var=nco_obj_typ_err;

    trv_tbl->lst[idx].is_crd_var=False;             
    trv_tbl->lst[idx].is_rec_var=False; 
    trv_tbl->lst[idx].var_typ=var_typ; 

    /* Variable dimensions */
    for(int dmn_idx_var=0;dmn_idx_var<NC_MAX_DIMS;dmn_idx_var++){
      trv_tbl->lst[idx].var_dmn[dmn_idx_var].dmn_nm=NULL;
      trv_tbl->lst[idx].var_dmn[dmn_idx_var].dmn_nm_fll=NULL;
      trv_tbl->lst[idx].var_dmn[dmn_idx_var].grp_nm_fll=NULL;
      trv_tbl->lst[idx].var_dmn[dmn_idx_var].is_crd_var=nco_obj_typ_err;
      trv_tbl->lst[idx].var_dmn[dmn_idx_var].crd=NULL;
      trv_tbl->lst[idx].var_dmn[dmn_idx_var].ncd=NULL;
      trv_tbl->lst[idx].var_dmn[dmn_idx_var].dmn_id=nco_obj_typ_err;
    }

     /* Variable dimensions; store what we know at this time: relative name and ID */
    for(int dmn_idx_var=0;dmn_idx_var<nbr_dmn_var;dmn_idx_var++){

      char dmn_nm_var[NC_MAX_NAME+1]; /* [sng] Dimension name */
      long dmn_sz_var;                /* [nbr] Dimension size */ 

      /* Get dimension name 
      nc_inq_dimname() currently returns only a simple dimension
      name, without a prefix identifying the group it came from.
      */
      (void)nco_inq_dim(grp_id,dmn_id_var[dmn_idx_var],dmn_nm_var,&dmn_sz_var);

      trv_tbl->lst[idx].var_dmn[dmn_idx_var].dmn_nm=strdup(dmn_nm_var);
      trv_tbl->lst[idx].var_dmn[dmn_idx_var].dmn_id=dmn_id_var[dmn_idx_var];

      if(dbg_lvl_get() == 13){
        (void)fprintf(stdout,"%s: INFO %s reports variable <%s> with dimension #%d'%s'\n",prg_nm_get(),fnc_nm,
          var_nm_fll,dmn_id_var[dmn_idx_var],dmn_nm_var);
      }
    }
 
    /* Free constructed name */
    var_nm_fll=(char *)nco_free(var_nm_fll);
  } /* end loop over variables */

  /* Add dimension objects */ 

  /* Iterate dimensions (for group; dimensions are defined *for* groups) */
  for(int dmn_idx=0;dmn_idx<nbr_dmn_grp;dmn_idx++){

    /* Keep the old table dimension size for insertion */
    idx=trv_tbl->nbr_dmn;

    /* Add one more element to dimension list of GTT (nco_realloc nicely handles first time/not first time insertions) */
    trv_tbl->nbr_dmn++;
    trv_tbl->lst_dmn=(dmn_trv_sct *)nco_realloc(trv_tbl->lst_dmn,trv_tbl->nbr_dmn*sizeof(dmn_trv_sct));

    /* Initialize dimension as a non-record dimension */
    trv_tbl->lst_dmn[idx].is_rec_dmn=False;

    /* Get dimension name */
    rcd+=nco_inq_dim(grp_id,dmn_ids_grp[dmn_idx],dmn_nm,&dmn_sz);

    /* Iterate unlimited dimensions to detect if dimension is record */
    for(int rec_idx=0;rec_idx<nbr_rec;rec_idx++){

      /* Get record dimension name */
      (void)nco_inq_dim(grp_id,dmn_ids_grp_ult[rec_idx],rec_nm,&rec_sz);

      /* Current dimension name matches current record dimension name ? */
      if(strcmp(rec_nm,dmn_nm) == 0 ){

        /* Dimension is a record dimension */
        trv_tbl->lst_dmn[idx].is_rec_dmn=True;

        /* Exit record dimension loop; we found it */
        break;
      } /* end match record dimension name */
    } /* end record dimension loop */

    /* Allocate path buffer and include space for trailing NUL */ 
    dmn_nm_fll=(char *)nco_malloc(strlen(grp_nm_fll)+strlen(dmn_nm)+2L);

    /* Initialize path with current absolute group path */
    strcpy(dmn_nm_fll,grp_nm_fll);

    /* If not root group, concatenate separator */
    if(strcmp(grp_nm_fll,sls_sng)) strcat(dmn_nm_fll,sls_sng);

    /* Concatenate dimension name to absolute group path */
    strcat(dmn_nm_fll,dmn_nm);

    /* Store object */
    strcpy(trv_tbl->lst_dmn[idx].nm,dmn_nm);            
    trv_tbl->lst_dmn[idx].grp_nm_fll=strdup(grp_nm_fll); 
    trv_tbl->lst_dmn[idx].nm_fll=strdup(dmn_nm_fll);    
    trv_tbl->lst_dmn[idx].sz=dmn_sz;                            
    trv_tbl->lst_dmn[idx].lmt_msa.dmn_nm=strdup(dmn_nm); 
    trv_tbl->lst_dmn[idx].lmt_msa.dmn_sz_org=dmn_sz;
    trv_tbl->lst_dmn[idx].lmt_msa.dmn_cnt=dmn_sz;
    trv_tbl->lst_dmn[idx].lmt_msa.WRP=False;
    trv_tbl->lst_dmn[idx].lmt_msa.BASIC_DMN=True;
    trv_tbl->lst_dmn[idx].lmt_msa.MSA_USR_RDR=False; 
    trv_tbl->lst_dmn[idx].lmt_msa.lmt_dmn_nbr=0;
    trv_tbl->lst_dmn[idx].lmt_msa.lmt_crr=0;
    trv_tbl->lst_dmn[idx].lmt_msa.lmt_dmn=NULL;
    trv_tbl->lst_dmn[idx].crd_nbr=0;         
    trv_tbl->lst_dmn[idx].crd=NULL; 
    trv_tbl->lst_dmn[idx].dmn_id=dmn_ids_grp[dmn_idx];
    trv_tbl->lst_dmn[idx].has_crd_scp=nco_obj_typ_err;

    /* Free constructed name */
    dmn_nm_fll=(char *)nco_free(dmn_nm_fll);
  } /* end dimension loop */

  /* Go to sub-groups */ 
  grp_ids=(int *)nco_malloc(nbr_grp*sizeof(int)); 
  rcd+=nco_inq_grps(grp_id,&nbr_grp,grp_ids);

  /* Heart of traversal construction: construct a new sub-group path and call function recursively with this new name; Voila */
  for(int grp_idx=0;grp_idx<nbr_grp;grp_idx++){
    char *sub_grp_nm_fll=NULL;  /* [sng] Sub group path */
    int gid=grp_ids[grp_idx];   /* [id] Current group ID */  

    /* Get sub-group name */
    rcd+=nco_inq_grpname(gid,grp_nm);

    /* Allocate path buffer including space for trailing NUL */ 
    sub_grp_nm_fll=(char *)nco_malloc(strlen(grp_nm_fll)+strlen(grp_nm)+2L);

    /* Initialize path with current absolute group path */
    strcpy(sub_grp_nm_fll,grp_nm_fll);

    /* If not root group, concatenate separator */
    if(strcmp(grp_nm_fll,sls_sng)) strcat(sub_grp_nm_fll,sls_sng);

    /* Concatenate current group to absolute group path */
    strcat(sub_grp_nm_fll,grp_nm); 

    /* Recursively process subgroups; NB: pass new absolute group name */
    rcd+=nco_grp_itr(gid,sub_grp_nm_fll,trv_tbl);

    /* Free constructed name */
    sub_grp_nm_fll=(char *)nco_free(sub_grp_nm_fll);
  } /* end loop over groups */

  (void)nco_free(grp_ids); 
  return rcd;
} /* end nco_grp_itr() */

void                      
nco_bld_crd_rec_var_trv               /* [fnc] Build dimension information for all variables */
(const trv_tbl_sct * const trv_tbl)   /* I [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Build "is_crd_var" and "is_rec_var" members for all variables */

  const char fnc_nm[]="nco_blb_crd_var_trv()"; /* [sng] Function name */

  /* Loop all objects */
  for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){
    trv_sct var_trv=trv_tbl->lst[var_idx];

    /* Interested in variables only */
    if(var_trv.nco_typ == nco_obj_typ_var){

      /* Loop unique dimensions list in groups */
      for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
        dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 

        /* Is there a variable with this dimension name anywhere? (relative name)  */
        if(strcmp(dmn_trv.nm,var_trv.nm) == 0 ){

          /* Is variable in scope of dimension ? */
          if(nco_var_dmn_scp(&var_trv,&dmn_trv,trv_tbl) == True ){

            /* Mark this variable as a coordinate variable */
            trv_tbl->lst[var_idx].is_crd_var=True;

            /* If the group dimension is a record dimension then the variable is a record variable */
            trv_tbl->lst[var_idx].is_rec_var=dmn_trv.is_rec_dmn;

            if(dbg_lvl_get() == nco_dbg_old){
              (void)fprintf(stdout,"%s: INFO %s <%s> is ",prg_nm_get(),fnc_nm,var_trv.nm_fll);
              if (dmn_trv.is_rec_dmn) (void)fprintf(stdout,"(record) ");
              (void)fprintf(stdout,"coordinate\n");
            }

            /* Go to next variable */
            break;

          }/* Is variable in scope of dimension ? */
        } /* Is there a variable with this dimension name anywhere? (relative name)  */
      } /* Loop unique dimensions list in groups */
    } /* Interested in variables only */
  } /* Loop all variables */

} /* nco_blb_crd_var_trv() */


void                      
nco_bld_crd_var_trv                   /* [fnc] Build GTT "crd_sct" coordinate variable structure */
(trv_tbl_sct * const trv_tbl)         /* I/O [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Build GTT "crd_sct" coordinate variable structure */

  const char fnc_nm[]="nco_blb_crd_var_trv()"; /* [sng] Function name */

  /* Step 1) Find the total number of coordinate variables for every dimension */

  /* Loop unique dimensions list in groups */
  for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
    dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 

    /* Loop all objects */
    for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){
      trv_sct var_trv=trv_tbl->lst[var_idx];

      /* Interested in variables only */
      if(var_trv.nco_typ == nco_obj_typ_var){

        /* Is there a variable with this dimension name anywhere? (relative name)  */
        if(strcmp(dmn_trv.nm,var_trv.nm) == 0 ){

          /* Is variable in scope of dimension ? */
          if(nco_var_dmn_scp(&var_trv,&dmn_trv,trv_tbl) == True ){

            /* Increment the number of coordinate variables for this dimension */
            trv_tbl->lst_dmn[dmn_idx].crd_nbr++;

          }/* Is variable in scope of dimension ? */
        } /* Is there a variable with this dimension name anywhere? (relative name)  */
      } /* Interested in variables only */
    } /* Loop all objects */
  } /* Loop unique dimensions list in groups */

  /* Step 2) Allocate coordinate variables array (crd_sct **) for every dimension */

  /* Loop unique dimensions list in groups */
  for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
    dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 

    /* Loop all objects */
    for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){
      trv_sct var_trv=trv_tbl->lst[var_idx];

      /* Interested in variables only */
      if(var_trv.nco_typ == nco_obj_typ_var){

        /* Is there a variable with this dimension name anywhere? (relative name)  */
        if(strcmp(dmn_trv.nm,var_trv.nm) == 0 ){

          /* Is variable in scope of dimension ? */
          if(nco_var_dmn_scp(&var_trv,&dmn_trv,trv_tbl) == True ){

            /* Total number of coordinate variables for this dimension */
            int crd_nbr=trv_tbl->lst_dmn[dmn_idx].crd_nbr;

            /* Alloc coordinate array if there are any coordinates */
            if (crd_nbr) trv_tbl->lst_dmn[dmn_idx].crd=(crd_sct **)nco_malloc(crd_nbr*sizeof(crd_sct *));

          }/* Is variable in scope of dimension ? */
        } /* Is there a variable with this dimension name anywhere? (relative name)  */
      } /* Interested in variables only */
    } /* Loop all objects */
  } /* Loop unique dimensions list in groups */

  /* Step 3) Allocate/Initialize every coordinate variable array for every dimension */

  /* Loop unique dimensions list in groups */
  for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
    dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 

    int crd_idx=0; /* [nbr] Coordinate index for current dimension */

    /* Loop all objects */
    for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){
      trv_sct var_trv=trv_tbl->lst[var_idx];

      /* Interested in variables only */
      if(var_trv.nco_typ == nco_obj_typ_var){

        /* Is there a variable with this dimension name anywhere? (relative name)  */
        if(strcmp(dmn_trv.nm,var_trv.nm) == 0 ){

          /* Is variable in scope of dimension ? */
          if(nco_var_dmn_scp(&var_trv,&dmn_trv,trv_tbl) == True ){

            /* Alloc this coordinate */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]=(crd_sct *)nco_malloc(sizeof(crd_sct));

            /* The coordinate full name is the variable full name found in scope */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->crd_nm_fll=strdup(var_trv.nm_fll);

            /* The coordinate dimension full name is the dimension full name */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->dmn_nm_fll=strdup(dmn_trv.nm_fll);

            /* The coordinate ID is the dimension unique ID */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->dmn_id=dmn_trv.dmn_id;

            /* Full group name where coordinate is located is the variable full group name  */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->crd_grp_nm_fll=strdup(var_trv.grp_nm_fll);

            /* Full group name where dimension of *this* coordinate is located is the full group name of the dimension  */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->dmn_grp_nm_fll=strdup(dmn_trv.grp_nm_fll);

            /* Store relative name (same for dimension and variable) */
            strcpy(trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->nm,var_trv.nm);

            /* Is a record dimension(variable) if the dimennsion is a record dimension */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->is_rec_dmn=dmn_trv.is_rec_dmn;

            /* Size is size */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->sz=dmn_trv.sz;

            /* Type */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->var_typ=var_trv.var_typ;

            /* Group depth */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->grp_dpt=var_trv.grp_dpt;


            /* MSA */     
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.dmn_nm=strdup(var_trv.nm);
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.dmn_cnt=dmn_trv.sz;
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.dmn_sz_org=dmn_trv.sz;
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.WRP=False;
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.BASIC_DMN=True;
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.MSA_USR_RDR=False;  
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_dmn_nbr=0;
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_crr=0;
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_dmn=NULL;

            crd_sct *crd=trv_tbl->lst_dmn[dmn_idx].crd[crd_idx];
            if(dbg_lvl_get() == nco_dbg_old){           
              (void)fprintf(stdout,"%s: INFO %s variable <%s> has coordinate <%s> from dimension <%s>\n",prg_nm_get(),fnc_nm,
                var_trv.nm_fll,crd->crd_nm_fll,crd->dmn_nm_fll);
            }

            /* Limits are initialized in build limits function */
        
            /* Incrementr coordinate index for current dimension */
            crd_idx++;

          }/* Is variable in scope of dimension ? */
        } /* Is there a variable with this dimension name anywhere? (relative name)  */
      } /* Interested in variables only */
    } /* Loop all objects */
  } /* Loop unique dimensions list in groups */

} /* nco_blb_crd_var_trv() */

void                          
nco_prt_trv_tbl                      /* [fnc] Print GTT (Group Traversal Table) for debugging  with --get_grp_info  */
(const int nc_id,                    /* I [ID] File ID */
 const trv_tbl_sct * const trv_tbl)  /* I [sct] GTT (Group Traversal Table) */
{
  /* Groups */

  int nbr_dmn;      /* [nbr] Total number of unique dimensions */
  int nbr_crd;      /* [nbr] Total number of coordinate variables */
  int nbr_crd_var;  /* [nbr] Total number of coordinate variables */

  nbr_dmn=0;
  (void)fprintf(stdout,"%s: INFO reports group information\n",prg_nm_get());
  for(unsigned grp_idx=0;grp_idx<trv_tbl->nbr;grp_idx++){

    /* Filter groups */
    if(trv_tbl->lst[grp_idx].nco_typ == nco_obj_typ_grp){
      trv_sct trv=trv_tbl->lst[grp_idx];            
      (void)fprintf(stdout,"%s: %d subgroups, %d dimensions, %d record dimensions, %d attributes, %d variables\n",
        trv.nm_fll,trv.nbr_grp,trv.nbr_dmn,trv.nbr_rec,trv.nbr_att,trv.nbr_var); 

      /* Print dimensions for group */
      (void)nco_prt_dmn(nc_id,trv.nm_fll);

      nbr_dmn+=trv.nbr_dmn;

    } /* Filter groups */
  } /* Loop groups */


  assert((unsigned int)nbr_dmn == trv_tbl->nbr_dmn);

  /* Variables */

  nbr_crd=0;
  (void)fprintf(stdout,"\n");
  (void)fprintf(stdout,"%s: INFO reports variable information\n",prg_nm_get());
  for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){

    /* Filter variables  */
    if(trv_tbl->lst[var_idx].nco_typ == nco_obj_typ_var){
      trv_sct trv=trv_tbl->lst[var_idx];   

      (void)fprintf(stdout,"%s:",trv.nm_fll); 

      /* Filter output */
      if (trv.is_crd_var == True){
        (void)fprintf(stdout," (coordinate)");
        nbr_crd++;
      }

      /* Filter output */
      if (trv.is_rec_var == True) (void)fprintf(stdout," (record)");

      /* If record variable must be coordinate variable */
      if (trv.is_rec_var == True) assert(trv.is_crd_var == True);

      (void)fprintf(stdout," %d dimensions: ",trv.nbr_dmn); 

      /* Full dimension names for each variable */
      for(int dmn_idx_var=0;dmn_idx_var<trv.nbr_dmn;dmn_idx_var++){

        /* Table can be printed before full dimension names are known, while debugging; in this case use relative name */
        if (trv.var_dmn[dmn_idx_var].dmn_nm_fll != NULL)
          (void)fprintf(stdout,"[%d]%s#%d ",dmn_idx_var,trv.var_dmn[dmn_idx_var].dmn_nm_fll,trv.var_dmn[dmn_idx_var].dmn_id); 
        else
          (void)fprintf(stdout,"[%d]%s#%d ",dmn_idx_var,trv.var_dmn[dmn_idx_var].dmn_nm,trv.var_dmn[dmn_idx_var].dmn_id); 

        /* Filter output */
        if (trv.var_dmn[dmn_idx_var].is_crd_var == True) (void)fprintf(stdout," (coordinate) : ");
      }

      (void)fprintf(stdout,"\n");

    } /* Filter variables  */
  } /* Variables */

  /* Unique dimension list, Coordinate variables stored in unique dimension list, limits */

  nbr_crd_var=0;
  (void)fprintf(stdout,"\n");
  (void)fprintf(stdout,"%s: INFO reports coordinate variables and limits listed by dimension:\n",prg_nm_get());
  for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
    dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 

    /* Dimension ID and  full name */
    (void)fprintf(stdout,"(#%d%s)",dmn_trv.dmn_id,dmn_trv.nm_fll);

    /* Filter output */
    if (dmn_trv.is_rec_dmn == True) (void)fprintf(stdout," record dimension(%li):: ",dmn_trv.sz);
    else if (dmn_trv.is_rec_dmn == False) (void)fprintf(stdout," dimension(%li):: ",dmn_trv.sz);

    nbr_crd_var+=dmn_trv.crd_nbr;

    /* Loop coordinates */
    for(int crd_idx=0;crd_idx<dmn_trv.crd_nbr;crd_idx++){
      crd_sct *crd=dmn_trv.crd[crd_idx];

      /* Coordinate full name */
      (void)fprintf(stdout,"%s ",crd->crd_nm_fll);

      /* Dimension full name */
      (void)fprintf(stdout,"(#%d%s) ",crd->dmn_id,crd->dmn_nm_fll);

      /* Limits */
      if (crd->lmt_msa.lmt_dmn_nbr){
        for(int lmt_idx=0;lmt_idx<crd->lmt_msa.lmt_dmn_nbr;lmt_idx++){ 
          lmt_sct *lmt_dmn=crd->lmt_msa.lmt_dmn[lmt_idx];
          (void)fprintf(stdout," [%d]%s(%li,%li,%li) ",lmt_idx,lmt_dmn->nm,lmt_dmn->srt,lmt_dmn->cnt,lmt_dmn->srd);
        }
      }/* Limits */

      /* Terminate this coordinate with "::" */
      if (dmn_trv.crd_nbr>1) (void)fprintf(stdout,":: ");

    }/* Loop coordinates */

    /* Terminate line */
    (void)fprintf(stdout,"\n");

  } /* Coordinate variables stored in unique dimension list */

  assert(nbr_crd_var == nbr_crd);

} /* nco_prt_trv_tbl() */

void
nco_bld_trv_tbl                       /* [fnc] Construct GTT, Group Traversal Table (groups,variables,dimensions, limits)   */
(const int nc_id,                     /* I [ID] netCDF file ID */
 char * const grp_pth,                /* I [sng] Absolute group path where to start build (root typically) */
 nco_bool MSA_USR_RDR,                /* I [flg] Multi-Slab Algorithm returns hyperslabs in user-specified order */
 int lmt_nbr,                         /* I [nbr] Number of user-specified dimension limits */
 lmt_sct **lmt,                       /* I [sct] Structure comming from nco_lmt_prs() */
 nco_bool FORTRAN_IDX_CNV,            /* I [flg] Hyperslab indices obey Fortran convention */
 trv_tbl_sct * const trv_tbl)         /* I/O [sct] Traversal table */
{
  /* Purpose: Construct GTT, Group Traversal Table (groups,variables,dimensions, limits) */

  /* Construct traversal table objects (groups,variables) */
  (void)nco_grp_itr(nc_id,grp_pth,trv_tbl);

  /* Print table in debug mode */
  if(dbg_lvl_get() == 13)(void)nco_prt_trv_tbl(nc_id,trv_tbl);

  /* Build dimension info for all variables (match dimension IDs) */
  (void)nco_bld_dmn_ids_trv(nc_id,trv_tbl);

  /* Build "is_crd_var" and "is_rec_var" members for all variables */
  (void)nco_bld_crd_rec_var_trv(trv_tbl);

  /* Build GTT "crd_sct" coordinate variable structure */
  (void)nco_bld_crd_var_trv(trv_tbl);

  /* Add dimension limits to traversal table; must be done before nco_bld_var_dmn() */
  if(lmt_nbr)(void)nco_bld_lmt(nc_id,MSA_USR_RDR,lmt_nbr,lmt,FORTRAN_IDX_CNV,trv_tbl);

  /* Variables in dimension's scope?   */
  (void)nco_has_crd_dmn_scp(trv_tbl);

  /* Assign variables dimensions to either coordinates or dimension structs; must be done last */
  (void)nco_bld_var_dmn(trv_tbl);

} /* nco_bld_trv_tbl() */



void
nco_bld_lmt                           /* [fnc] Assign user specified dimension limits to traversal table */
(const int nc_id,                     /* I [ID] netCDF file ID */
 nco_bool MSA_USR_RDR,                /* I [flg] Multi-Slab Algorithm returns hyperslabs in user-specified order */
 int lmt_nbr,                         /* I [nbr] Number of user-specified dimension limits */
 lmt_sct **lmt,                       /* I [sct] Structure comming from nco_lmt_prs() */
 nco_bool FORTRAN_IDX_CNV,            /* I [flg] Hyperslab indices obey Fortran convention */
 trv_tbl_sct * const trv_tbl)         /* I/O [sct] Traversal table */
{
  /* Purpose: Assign user-specified dimension limits to traversal table  
  At this point "lmt" was parsed from nco_lmt_prs(); only the relative names and  min, max, stride are known 
  Steps:

  Step 1) Find the total numbers of matches for a dimension
  ncks -d lon,0,0,1  ~/nco/data/in_grp.nc
  Here "lmt_nbr" is 1 and there is 1 match at most
  ncks -d lon,0,0,1 -d lon,0,0,1 -d lat,0,0,1  ~/nco/data/in_grp.nc
  Here "lmt_nbr" is 3 and there are 2 matches at most for "lon" and 1 match at most for "lat"
  The limits have to be separated to 
  a) case of coordinate variables
  b) case of dimension only (there is no coordinate variable for that dimension)

  Step 2) Allocate and initialize counter index for number of limits to zero for a dimension;  
  "lmt_dmn_nbr" needed from Step 1; initialize dimension structure limit information

  Step 3) Deep copy matches to table, match at the current index, increment current index
  [ID] Dimension ID is set to -1 (Traversal code should be ID free)

  Step 4) Apply MSA for each Dimension in a new cycle (that now has all its limits in place :-) ) 
  At this point lmt_sct is no longer needed;  

  Tests:
  ncks -D 11 -d lon,0,0,1 -d lon,1,1,1 -d lat,0,0,1 -d time,1,2,1 -d time,6,7,1 -v lon,lat,time -H ~/nco/data/in_grp.nc
  ncks -D 11 -d time,8,9 -d time,0,2  -v time -H ~/nco/data/in_grp.nc
  ncks -D 11 -d time,8,2 -v time -H ~/nco/data/in_grp.nc # wrapped limit
  */

  const char fnc_nm[]="nco_bld_lmt()"; /* [sng] Function name  */

  if(dbg_lvl_get() == nco_dbg_old){
    (void)fprintf(stdout,"%s: INFO %s reports %d input dimension limits: ",prg_nm_get(),fnc_nm,lmt_nbr);
    for(int lmt_idx=0;lmt_idx<lmt_nbr;lmt_idx++)(void)fprintf(stdout,"[%d]%s: ",lmt_idx,lmt[lmt_idx]->nm);
    (void)fprintf(stdout,"\n");      
  } /* endif dbg */

  /* Step 1) Find the total numbers of limit matches for a dimension and/or a coordinate variable */

  /* Loop input name list (can have duplicate names)  */
  for(int lmt_idx=0;lmt_idx<lmt_nbr;lmt_idx++){

    /* Loop dimensions  */
    for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
      dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 

      /*  The limits have to be separated to */

      /* a) case where the dimension has coordinate variables */
      if (dmn_trv.crd_nbr){

        /* Loop coordinates */
        for(int crd_idx=0;crd_idx<dmn_trv.crd_nbr;crd_idx++){
          crd_sct *crd=dmn_trv.crd[crd_idx];

          /* Match input *relative* name to coordinate relative name */ 
          if(strcmp(lmt[lmt_idx]->nm,crd->nm) == 0){

            /* Increment number of dimension limits this coordinate */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_dmn_nbr++;

          } /* End Match input name to table name */ 
        }/* Loop coordinates */
      }else{
        /* b) case of dimension only (there is no coordinate variable for this dimension */

        /* Match input *relative* name to dimension relative name */ 
        if(strcmp(lmt[lmt_idx]->nm,dmn_trv.nm) == 0){

          /* Increment number of dimension limits for this dimension */
          trv_tbl->lst_dmn[dmn_idx].lmt_msa.lmt_dmn_nbr++;

        } /* Match input *relative* name to dimension relative name */ 
      } /* b) case of dimension only (there is no coordinate variable for this dimension */
    } /* Loop dimensions  */
  } /* Loop input name list (can have duplicate names)  */


  /* Step 2) Initialize MSA for all dimensions, allocate lmt_sct ** */

  /* Loop dimensions, that now have already distributed limits and initialize limit information */
  for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
    dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx];

    /*  The limits are now separated to */

    /* a) case where the dimension has coordinate variables */
    if (dmn_trv.crd_nbr){

      /* Loop coordinates */
      for(int crd_idx=0;crd_idx<dmn_trv.crd_nbr;crd_idx++){
        crd_sct *crd=dmn_trv.crd[crd_idx];

        /* Alloc limits if there are any */
        if (crd->lmt_msa.lmt_dmn_nbr) trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_dmn=(lmt_sct **)nco_malloc(crd->lmt_msa.lmt_dmn_nbr*sizeof(lmt_sct *));

      }/* Loop coordinates */
    }else{
      /* b) case of dimension only (there is no coordinate variable for this dimension */  

      /* Alloc limits if there are any */
      if (dmn_trv.lmt_msa.lmt_dmn_nbr){
        trv_tbl->lst_dmn[dmn_idx].lmt_msa.lmt_dmn=(lmt_sct **)nco_malloc(dmn_trv.lmt_msa.lmt_dmn_nbr*sizeof(lmt_sct *));
      }

    } /* b) case of dimension only (there is no coordinate variable for this dimension */
  } /* Loop dimensions, that now have already distributed limits and initialize limit information */


  /* Step 3) Store matches in table, match at the current index, increment current index  */

  /* Loop input name list (can have duplicate names)  */
  for(int lmt_idx=0;lmt_idx<lmt_nbr;lmt_idx++){

    /* Loop dimensions  */
    for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
      dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 

      /*  The limits have to be separated to */

      /* a) case where the dimension has coordinate variables */
      if (dmn_trv.crd_nbr){

        /* Loop coordinates */
        for(int crd_idx=0;crd_idx<dmn_trv.crd_nbr;crd_idx++){
          crd_sct *crd=dmn_trv.crd[crd_idx];

          /* Match input *relative* name to coordinate relative name */ 
          if(strcmp(lmt[lmt_idx]->nm,crd->nm) == 0){

            /* Limit is same as dimension in input file ? */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.BASIC_DMN=False;

            /* Parse user-specified limits into hyperslab specifications. NOTE: Use True parameter and "crd" */
            (void)nco_lmt_evl_dmn_crd(nc_id,0L,FORTRAN_IDX_CNV,crd->crd_grp_nm_fll,crd->nm,crd->sz,crd->is_rec_dmn,True,lmt[lmt_idx]);

            if(dbg_lvl_get() == nco_dbg_old){
              (void)fprintf(stdout,"%s: INFO %s dimension [%d]%s done (%li->%li) insert in table at [%d]%s:\n",
                prg_nm_get(),fnc_nm,lmt_idx,lmt[lmt_idx]->nm,lmt[lmt_idx]->min_idx,lmt[lmt_idx]->max_idx,dmn_idx,dmn_trv.nm_fll);
            }

            /* Current index (lmt_crr) of dimension limits for this (dmn_idx) table dimension  */
            int lmt_crr=trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_crr;

            /* Increment current index being initialized  */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_crr++;

            /* Alloc this limit */
            trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_dmn[lmt_crr]=(lmt_sct *)nco_malloc(sizeof(lmt_sct));

            /* Initialize this entry */
            (void)nco_lmt_init(trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_dmn[lmt_crr]);

            /* Store this valid input; deep-copy to table */ 
            (void)nco_lmt_cpy(lmt[lmt_idx],trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_dmn[lmt_crr]);

            /* Print copy in table */ 
            if(dbg_lvl_get() == nco_dbg_old){
              (void)nco_lmt_prt(trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_dmn[lmt_crr]);
            }

          } /* End Match input name to table name */ 
        }/* Loop coordinates */
      }else{
        /* b) case of dimension only (there is no coordinate variable for this dimension) */

        /* Match input *relative* name to dimension relative name */ 
        if(strcmp(lmt[lmt_idx]->nm,dmn_trv.nm) == 0){

          /* Limit is same as dimension in input file ? */
          trv_tbl->lst_dmn[dmn_idx].lmt_msa.BASIC_DMN=False;

          if(dbg_lvl_get() == nco_dbg_old)(void)fprintf(stdout,"%s: INFO %s dimension <%s> found:\n",prg_nm_get(),fnc_nm,dmn_trv.nm_fll);

          /* Parse user-specified limits into hyperslab specifications. NOTE: Use False parameter and "dmn" */
          (void)nco_lmt_evl_dmn_crd(nc_id,0L,FORTRAN_IDX_CNV,dmn_trv.grp_nm_fll,dmn_trv.nm,dmn_trv.sz,dmn_trv.is_rec_dmn,False,lmt[lmt_idx]);

          if(dbg_lvl_get() == nco_dbg_old){
            (void)fprintf(stdout,"%s: INFO %s dimension [%d]%s done (%li->%li) insert in table at [%d]%s:\n",
              prg_nm_get(),fnc_nm,lmt_idx,lmt[lmt_idx]->nm,lmt[lmt_idx]->min_idx,lmt[lmt_idx]->max_idx,dmn_idx,dmn_trv.nm_fll);
          }

          /* Current index (lmt_crr) of dimension limits for this (dmn_idx) table dimension  */
          int lmt_crr=trv_tbl->lst_dmn[dmn_idx].lmt_msa.lmt_crr;

          /* Increment current index being initialized  */
          trv_tbl->lst_dmn[dmn_idx].lmt_msa.lmt_crr++;

          /* Alloc this limit */
          trv_tbl->lst_dmn[dmn_idx].lmt_msa.lmt_dmn[lmt_crr]=(lmt_sct *)nco_malloc(sizeof(lmt_sct));

          /* Initialize this entry */
          (void)nco_lmt_init(trv_tbl->lst_dmn[dmn_idx].lmt_msa.lmt_dmn[lmt_crr]);

          /* Store this valid input; deep-copy to table */ 
          (void)nco_lmt_cpy(lmt[lmt_idx],trv_tbl->lst_dmn[dmn_idx].lmt_msa.lmt_dmn[lmt_crr]);

          /* Print copy in table */ 
          if(dbg_lvl_get() == nco_dbg_old){
            (void)nco_lmt_prt(trv_tbl->lst_dmn[dmn_idx].lmt_msa.lmt_dmn[lmt_crr]);
          }

        } /* Match input *relative* name to dimension relative name */ 
      } /* b) case of dimension only (there is no coordinate variable for this dimension */
    } /* Loop dimensions  */
  } /* Loop input name list (can have duplicate names)  */


  /* Step 4) Apply MSA for each Dimension in a new cycle (that now has all its limits in place)  */

  /* Loop dimensions  */
  for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
    dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 

    /*  The limits have to be separated to */

    /* a) case where the dimension has coordinate variables */
    if (dmn_trv.crd_nbr){

      /* Loop coordinates */
      for(int crd_idx=0;crd_idx<dmn_trv.crd_nbr;crd_idx++){
        crd_sct *crd=dmn_trv.crd[crd_idx];

        /* Adapted from the original MSA loop in nco_msa_lmt_all_ntl(); differences are marked GTT specific */

        nco_bool flg_ovl; /* [flg] Limits overlap */

        /* GTT: If this coordinate has no limits, continue */
        if (crd->lmt_msa.lmt_dmn_nbr == 0) continue;

        /* ncra/ncrcat have only one limit for record dimension so skip evaluation otherwise this messes up multi-file operation */
        if(crd->is_rec_dmn && (prg_get() == ncra || prg_get() == ncrcat)) continue;

        /* Split-up wrapped limits. NOTE: using deep copy version nco_msa_wrp_splt_cpy() */   
        (void)nco_msa_wrp_splt_cpy(&trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa);

        /* Wrapped hyperslabs are dimensions broken into the "wrong" order,e.g. from
        -d time,8,2 broken into -d time,8,9 -d time,0,2 
        WRP flag set only when list contains dimensions split as above */
        if(trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.WRP == True){

          /* Find and store size of output dim */  
          (void)nco_msa_clc_cnt(&trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa); 

          continue;
        } /* End WRP flag set */

        /* Single slab---no analysis needed */  
        if(trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.lmt_dmn_nbr == 1){

          (void)nco_msa_clc_cnt(&trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa);  

          continue;    
        } /* End Single slab */

        /* Does Multi-Slab Algorithm returns hyperslabs in user-specified order ? */
        if(MSA_USR_RDR){
          trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa.MSA_USR_RDR=True;

          /* Find and store size of output dimension */  
          (void)nco_msa_clc_cnt(&trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa);  

          continue;
        } /* End MSA_USR_RDR */

        /* Sort limits */
        (void)nco_msa_qsort_srt(&trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa);

        /* Check for overlap */
        flg_ovl=nco_msa_ovl(&trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa); 

        /* Find and store size of output dimension */  
        (void)nco_msa_clc_cnt(&trv_tbl->lst_dmn[dmn_idx].crd[crd_idx]->lmt_msa);

        if(dbg_lvl_get() > 1){
          if(flg_ovl) (void)fprintf(stdout,"%s: coordinate \"%s\" has overlapping hyperslabs\n",prg_nm_get(),crd->nm); 
          else (void)fprintf(stdout,"%s: coordinate \"%s\" has distinct hyperslabs\n",prg_nm_get(),crd->nm); 
        } 

      }/* Loop coordinates */
    }else{
      /* b) case of dimension only (there is no coordinate variable for this dimension) */

      /* Adapted from the original MSA loop in nco_msa_lmt_all_ntl(); differences are marked GTT specific */

      nco_bool flg_ovl; /* [flg] Limits overlap */

      /* GTT: If this dimension has no limits, continue */
      if (trv_tbl->lst_dmn[dmn_idx].lmt_msa.lmt_dmn_nbr == 0) continue;

      /* ncra/ncrcat have only one limit for record dimension so skip evaluation otherwise this messes up multi-file operation */
      if(trv_tbl->lst_dmn[dmn_idx].is_rec_dmn && (prg_get() == ncra || prg_get() == ncrcat)) continue;

      /* Split-up wrapped limits */   
      (void)nco_msa_wrp_splt_trv(&trv_tbl->lst_dmn[dmn_idx]);

      /* Wrapped hyperslabs are dimensions broken into the "wrong" order,e.g. from
      -d time,8,2 broken into -d time,8,9 -d time,0,2 
      WRP flag set only when list contains dimensions split as above */
      if(trv_tbl->lst_dmn[dmn_idx].lmt_msa.WRP == True){

        /* Find and store size of output dim */  
        (void)nco_msa_clc_cnt_trv(&trv_tbl->lst_dmn[dmn_idx]); 

        continue;
      } /* End WRP flag set */

      /* Single slab---no analysis needed */  
      if(trv_tbl->lst_dmn[dmn_idx].lmt_msa.lmt_dmn_nbr == 1){

        (void)nco_msa_clc_cnt_trv(&trv_tbl->lst_dmn[dmn_idx]);  

        continue;    
      } /* End Single slab */

      /* Does Multi-Slab Algorithm returns hyperslabs in user-specified order ? */
      if(MSA_USR_RDR){
        trv_tbl->lst_dmn[dmn_idx].lmt_msa.MSA_USR_RDR=True;

        /* Find and store size of output dimension */  
        (void)nco_msa_clc_cnt_trv(&trv_tbl->lst_dmn[dmn_idx]);  

        continue;
      } /* End MSA_USR_RDR */

      /* Sort limits */
      (void)nco_msa_qsort_srt_trv(&trv_tbl->lst_dmn[dmn_idx]);

      /* Check for overlap */
      flg_ovl=nco_msa_ovl_trv(&trv_tbl->lst_dmn[dmn_idx]);  

      if(flg_ovl==False) trv_tbl->lst_dmn[dmn_idx].lmt_msa.MSA_USR_RDR=True;

      /* Find and store size of output dimension */  
      (void)nco_msa_clc_cnt_trv(&trv_tbl->lst_dmn[dmn_idx]);

      if(dbg_lvl_get() > 1){
        if(flg_ovl) (void)fprintf(stdout,"%s: dimension \"%s\" has overlapping hyperslabs\n",prg_nm_get(),trv_tbl->lst_dmn[dmn_idx].nm); 
        else (void)fprintf(stdout,"%s: dimension \"%s\" has distinct hyperslabs\n",prg_nm_get(),trv_tbl->lst_dmn[dmn_idx].nm); 
      } 

    } /* b) case of dimension only (there is no coordinate variable for this dimension */
  } /* Loop dimensions  */


  /* Step 4) Validate...need more here */

#ifdef NCO_SANITY_CHECK
  /* Loop table dimensions */
  for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
    dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 

    /*  The limits have to be separated to */

    /* a) case where the dimension has coordinate variables */
    if (dmn_trv.crd_nbr){

      /* Loop coordinates */
      for(int crd_idx=0;crd_idx<dmn_trv.crd_nbr;crd_idx++){
        crd_sct *crd=dmn_trv.crd[crd_idx];

        if(dbg_lvl_get() == nco_dbg_old && crd->lmt_msa.lmt_dmn_nbr){
          (void)fprintf(stdout,"%s: INFO %s checking limits for coordinate <%s>:\n",prg_nm_get(),fnc_nm,crd->crd_nm_fll);
        }

        /* lmt_dmn_nbr can be incremented for wrapped limits; always sync   */
        assert(crd->lmt_msa.lmt_crr == crd->lmt_msa.lmt_dmn_nbr);

        /* Loop limits for each coordinate */
        for(int lmt_idx=0;lmt_idx<crd->lmt_msa.lmt_dmn_nbr;lmt_idx++){
          if(dbg_lvl_get() == nco_dbg_old){
            (void)fprintf(stdout,"%s: INFO %s checking limit[%d]:%s:(%li,%li,%li)\n",prg_nm_get(),fnc_nm,
              lmt_idx,
              crd->lmt_msa.lmt_dmn[lmt_idx]->nm,
              crd->lmt_msa.lmt_dmn[lmt_idx]->srt,
              crd->lmt_msa.lmt_dmn[lmt_idx]->end,
              crd->lmt_msa.lmt_dmn[lmt_idx]->srd);
          }

          /* Need more MRA sanity checks here; checking srt <= end now */
          assert(crd->lmt_msa.lmt_dmn[lmt_idx]->srt <= crd->lmt_msa.lmt_dmn[lmt_idx]->end);
          assert(crd->lmt_msa.lmt_dmn[lmt_idx]->srd >= 1);
        }/* End Loop limits for each dimension */

      }/* Loop coordinates */
    }else{

      /* Number of dimension limits for table dimension  */
      int lmt_dmn_nbr=dmn_trv.lmt_msa.lmt_dmn_nbr;

      /* Current index of dimension limits for table dimension  */
      int lmt_crr=dmn_trv.lmt_msa.lmt_crr;

      if(dbg_lvl_get() == nco_dbg_old && lmt_dmn_nbr){
        (void)fprintf(stdout,"%s: INFO %s checking limits for dimension <%s>:\n",prg_nm_get(),fnc_nm,dmn_trv.nm_fll);
      }

      /* lmt_dmn_nbr can be incremented for wrapped limits; always sync   */
      assert(lmt_crr == lmt_dmn_nbr);

      /* Loop limits for each dimension */
      for(int lmt_idx=0;lmt_idx<dmn_trv.lmt_msa.lmt_dmn_nbr;lmt_idx++){
        if(dbg_lvl_get() == nco_dbg_old){
          (void)fprintf(stdout,"%s: INFO %s checking limit[%d]:%s:(%li,%li,%li)\n",prg_nm_get(),fnc_nm,
            lmt_idx,
            dmn_trv.lmt_msa.lmt_dmn[lmt_idx]->nm,
            dmn_trv.lmt_msa.lmt_dmn[lmt_idx]->srt,
            dmn_trv.lmt_msa.lmt_dmn[lmt_idx]->end,
            dmn_trv.lmt_msa.lmt_dmn[lmt_idx]->srd);
        }

        /* Need more MRA sanity checks here; checking srt <= end now */
        assert(dmn_trv.lmt_msa.lmt_dmn[lmt_idx]->srt <= dmn_trv.lmt_msa.lmt_dmn[lmt_idx]->end);
        assert(dmn_trv.lmt_msa.lmt_dmn[lmt_idx]->srd >= 1);
      }/* End Loop limits for each dimension */


    } /* b) case of dimension only (there is no coordinate variable for this dimension */
  } /* Loop dimensions  */

#endif /* NCO_SANITY_CHECK */


} /* nco_bld_lmt() */

void                          
nco_has_crd_dmn_scp                  /* [fnc] Is there a variable with same name in dimension's scope?   */
(const trv_tbl_sct * const trv_tbl)  /* I [sct] GTT (Group Traversal Table) */
{

  const char fnc_nm[]="nco_has_crd_dmn_scp()"; /* [sng] Function name  */

  /* Unique dimension list */

  if(dbg_lvl_get() == nco_dbg_old)(void)fprintf(stdout,"%s: INFO reports dimension information with limits: %d dimensions\n",prg_nm_get(),trv_tbl->nbr_dmn);
  for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
    dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 

    /* Dimension #/name first */
    if(dbg_lvl_get() == nco_dbg_old) (void)fprintf(stdout,"#%d%s\n",dmn_trv.dmn_id,dmn_trv.nm_fll);

    nco_bool in_scp=False;

    /* Loop object table */
    for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){

      /* Filter variables  */
      if(trv_tbl->lst[var_idx].nco_typ == nco_obj_typ_var){
        trv_sct var_trv=trv_tbl->lst[var_idx];   

        /* Is there a variable with this dimension name (a coordinate varible) anywhere (relative name)  */
        if(strcmp(dmn_trv.nm,var_trv.nm) == 0){

          /* Is variable in scope of dimension ? */
          if(nco_var_dmn_scp(&var_trv,&dmn_trv,trv_tbl) == True ){

            if(dbg_lvl_get() == nco_dbg_old){
              (void)fprintf(stdout,"%s: INFO %s reports variable <%s> in scope of dimension <%s>\n",prg_nm_get(),fnc_nm,
                var_trv.nm_fll,dmn_trv.nm_fll);        
            } /* endif dbg */

            trv_tbl->lst_dmn[dmn_idx].has_crd_scp=True;

            /* Built before; variable must be a cordinate */
            assert(var_trv.is_crd_var == True);

            in_scp=True;

          } /* Is variable in scope of dimension ? */
        } /* Is there a variable with this dimension name anywhere? (relative name)  */
      } /* Filter variables  */
    } /* Loop object table */


    if(dbg_lvl_get() == nco_dbg_old){
      if (in_scp == False)
        (void)fprintf(stdout,"%s: INFO %s dimension <%s> with no in scope variables\n",prg_nm_get(),fnc_nm,
        dmn_trv.nm_fll);        
    } /* endif dbg */

    trv_tbl->lst_dmn[dmn_idx].has_crd_scp=in_scp;

  } /* Unique dimension list */

  /* Unique dimension list */
  for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
    dmn_trv_sct dmn_trv=trv_tbl->lst_dmn[dmn_idx]; 
    assert(dmn_trv.has_crd_scp != nco_obj_typ_err);
  } /* Unique dimension list */

} /* nco_has_crd_dmn_scp() */

nco_bool                               /* O [flg] True if variable is in scope of dimension */
nco_var_dmn_scp                        /* [fnc] Is variable in dimension scope */
(const trv_sct * const var_trv,        /* I [sct] GTT Object Variable */
 const dmn_trv_sct * const dmn_trv,    /* I [sct] GTT unique dimension */
 const trv_tbl_sct * const trv_tbl)    /* I [sct] GTT (Group Traversal Table) */
{
  /* Purpose: Find if variable is in scope of the dimension: 
  Use case in scope:
  dimension /lon 
  variable /g1/lon
  Use case not in scope:
  variable /lon
  dimension /g1/lon

  NOTE: deal with cases like
  dimension: /lon
  variable:  /g8/lon 
  dimension: /g8/lon
  */

  const char fnc_nm[]="nco_var_dmn_scp()";   /* [sng] Function name */

  const char sls_chr='/';                    /* [chr] Slash character */
  char *sbs_srt;                             /* [sng] Location of user-string match start in object path */
  char *sbs_end;                             /* [sng] Location of user-string match end   in object path */

  nco_bool flg_pth_srt_bnd=False;            /* [flg] String begins at path component boundary */
  nco_bool flg_pth_end_bnd=False;            /* [flg] String ends   at path component boundary */

  size_t var_sng_lng;                        /* [nbr] Length of variable name */
  size_t var_nm_fll_lng;                     /* [nbr] Length of full variable name */
  size_t dmn_nm_fll_lng;                     /* [nbr] Length of of full dimension name */

  /* Most common case is for the unique dimension full name to match the full variable name   */
  if (strcmp(var_trv->nm_fll,dmn_trv->nm_fll) == 0){
    if(dbg_lvl_get() == 12){
      (void)fprintf(stdout,"%s: INFO %s found absolute match of variable <%s> and dimension <%s>:\n",prg_nm_get(),fnc_nm,
        var_trv->nm_fll,dmn_trv->nm_fll);
    }
    return True;
  }

  /* Deal with in scope cases */

  var_nm_fll_lng=strlen(var_trv->nm_fll);
  dmn_nm_fll_lng=strlen(dmn_trv->nm_fll);
  var_sng_lng=strlen(var_trv->nm);

  /* Look for partial match, not necessarily on path boundaries; locate variable (str2) in full dimension name (str1) */
  if((sbs_srt=strstr(dmn_trv->nm_fll,var_trv->nm))){

    /* Ensure match spans (begins and ends on) whole path-component boundaries */

    /* Does match begin at path component boundary ... directly on a slash? */
    if(*sbs_srt == sls_chr){
      flg_pth_srt_bnd=True;
    }

    /* ...or one after a component boundary? */
    if((sbs_srt > dmn_trv->nm_fll) && (*(sbs_srt-1L) == sls_chr)){
      flg_pth_srt_bnd=True;
    }

    /* Does match end at path component boundary ... directly on a slash? */
    sbs_end=sbs_srt+var_sng_lng-1L;

    if(*sbs_end == sls_chr){
      flg_pth_end_bnd=True;
    }

    /* ...or one before a component boundary? */
    if(sbs_end <= dmn_trv->nm_fll+dmn_nm_fll_lng-1L){
      if((*(sbs_end+1L) == sls_chr) || (*(sbs_end+1L) == '\0')){
        flg_pth_end_bnd=True;
      }
    }

    /* If match is on both ends of '/' then it's a "real" name, not for example "lat_lon" as a variable looking for "lon" */
    if (flg_pth_srt_bnd && flg_pth_end_bnd){

      /* Absolute match (equality redundant); strcmp deals cases like /g3/rlev/ and /g5/rlev  */
      if (var_nm_fll_lng == dmn_nm_fll_lng && strcmp(var_trv->nm_fll,dmn_trv->nm_fll) == 0){
        if(dbg_lvl_get() == 12){
          (void)fprintf(stdout,"%s: INFO %s found absolute match of variable <%s> and dimension <%s>:\n",prg_nm_get(),fnc_nm,
            var_trv->nm_fll,dmn_trv->nm_fll);
        }
        return True;

        /* Variable in scope of dimension */
      }else if (var_nm_fll_lng>dmn_nm_fll_lng){

        /* NOTE: deal with cases like
        dimension: /lon
        variable:  /g8/lon 
        dimension: /g8/lon
        */

        /* Loop unique dimensions list in groups */
        for(unsigned dmn_idx=0;dmn_idx<trv_tbl->nbr_dmn;dmn_idx++){
          dmn_trv_sct dmn=trv_tbl->lst_dmn[dmn_idx]; 
          /* Loop all objects */
          for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){
            trv_sct var=trv_tbl->lst[var_idx];
            /* Interested in variables only */
            if(var.nco_typ == nco_obj_typ_var){
              /* Is there a *full* match already for the *input* dimension ?  */
              if(strcmp(var_trv->nm_fll,dmn.nm_fll) == 0 ){
                if(dbg_lvl_get() == 12){
                  (void)fprintf(stdout,"%s: INFO %s variable <%s> has another dimension full match <%s>:\n",prg_nm_get(),fnc_nm,
                    var_trv->nm_fll,dmn.nm_fll);
                }
                return False;
              } /* Is there a *full* match already?  */
            } /* Interested in variables only */
          } /* Loop all objects */
        } /* Loop unique dimensions list in groups */


        if(dbg_lvl_get() == 12){
          (void)fprintf(stdout,"%s: INFO %s found variable <%s> in scope of dimension <%s>:\n",prg_nm_get(),fnc_nm,
            var_trv->nm_fll,dmn_trv->nm_fll);
        }
        return True;

        /* Variable out of scope of dimension */
      }else if (var_nm_fll_lng < dmn_nm_fll_lng){
        if(dbg_lvl_get() == 12){
          (void)fprintf(stdout,"%s: INFO %s found variable <%s> out of scope of dimension <%s>:\n",prg_nm_get(),fnc_nm,
            var_trv->nm_fll,dmn_trv->nm_fll);
        }
        return False;

      } /* Absolute match  */
    } /* If match is on both ends of '/' then it's a "real" name, not for example "lat_lon" as a variable looking for "lon" */
  }/* Look for partial match, not necessarily on path boundaries */

  return False;
} /* nco_var_dmn_scp() */

int                                  /* O [nbr] Comparison result */
nco_cmp_crd_dpt                      /* [fnc] Compare two crd_sct's by group depth */
(const void *p1,                     /* I [sct] crd_sct* to compare */
 const void *p2)                     /* I [sct] crd_sct* to compare */
{
  /* Purpose: Compare two crd_sct's by group depth structure member, used by qsort()
     crd_sct **crd of unique dimension of is an array of pointers, not an array of crd_sct structs */

  crd_sct **crd1=(crd_sct **)p1;
  crd_sct **crd2=(crd_sct **)p2;

  if ( (*crd1)->grp_dpt > (*crd2)->grp_dpt ) return -1;
  else if ( (*crd1)->grp_dpt < (*crd2)->grp_dpt ) return 1;
  else return 0;

} /* end nco_cmp_crd_dpt() */


crd_sct *                             /* O [sct] Coordinate object */
nco_scp_var_crd                       /* [fnc] Return in scope coordinate for variable  */
(trv_sct *var_trv,                    /* I [sct] Variable object */
 dmn_trv_sct *dmn_trv)                /* I [sct] Dimension object */
{

  /* Purpose: Choose one coordinate from the dimension object to assign as a valid coordinate
  to the variable dimension
  Scope definition: In the same group of the variable or beneath (closer to root) 
  Above: out of scope (no luck)

  Use cases:

  dimension lon4;
  variable lon4_var(lon4)

  Variable /g16/g16g4/g16g4g4/g16g4g4g4/lon4_var
  2 coordinates down in scope 
  /g16/g16g4/g16g4g4/lon4
  /g16/g16g4/lon4

  */

  /* If more then 1 coordinate, sort them by group depth */
  if (dmn_trv->crd_nbr>1){
    qsort(dmn_trv->crd,(size_t)dmn_trv->crd_nbr,sizeof(crd_sct *),nco_cmp_crd_dpt);
  }

  /* Loop coordinates; they all have the unique dimension ID of the variable dimension */
  for(int crd_idx=0;crd_idx<dmn_trv->crd_nbr;crd_idx++){
    crd_sct *crd=dmn_trv->crd[crd_idx];

    /* Absolute match: in scope  */ 
    if (strcmp(var_trv->nm_fll,crd->crd_nm_fll) == 0){ 

      /* The variable must be a coordinate for this to happen */
      assert(var_trv->is_crd_var == True);
      return crd;
    } 
    /* Same group: in scope  */ 
    else if (strcmp(var_trv->grp_nm_fll,crd->crd_grp_nm_fll) == 0){ 
      return crd;
    } 
    /* Level below: in scope  */ 
    else if (crd->grp_dpt < var_trv->grp_dpt){ 
      return crd;
    }
  } /* Loop coordinates */

  return NULL;
} /* nco_scp_var_crd() */



void
nco_bld_var_dmn                       /* [fnc] Assign variables dimensions to either coordinates or dimension structs */
(trv_tbl_sct * const trv_tbl)         /* I/O [sct] Traversal table */
{
  /* Purpose: Fill variable dimensions with pointers to either a coordinate variable or dimension structs  */

  const char fnc_nm[]="nco_bld_var_dmn()"; /* [sng] Function name  */

  /* Loop table */
  for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){

    /* Filter variables  */
    if(trv_tbl->lst[var_idx].nco_typ == nco_obj_typ_var){
      trv_sct var_trv=trv_tbl->lst[var_idx];   

      /* Loop dimensions for object (variable)  */
      for(int dmn_idx_var=0;dmn_idx_var<var_trv.nbr_dmn;dmn_idx_var++) {

        /* Get unique dimension ID from variable dimension */
        int var_dmn_id=var_trv.var_dmn[dmn_idx_var].dmn_id;

        /* Get unique dimension object from unique dimension ID */
        dmn_trv_sct *dmn_trv=nco_dmn_trv_sct(var_dmn_id,trv_tbl);

        /* No coordinates */
        if(dmn_trv->crd_nbr == 0) {

          if(dbg_lvl_get() == 13){
            (void)fprintf(stdout,"%s: INFO %s reports variable <%s> with *NON* coordinate dimension [%d]%s\n",prg_nm_get(),fnc_nm,
              var_trv.nm_fll,dmn_idx_var,var_trv.var_dmn[dmn_idx_var].dmn_nm_fll);        
          } /* endif dbg */

          /* Mark as False the position of the bool array coordinate/non coordinate */
          trv_tbl->lst[var_idx].var_dmn[dmn_idx_var].is_crd_var=False;

          /* Store unique dimension (non coordinate ) */
          trv_tbl->lst[var_idx].var_dmn[dmn_idx_var].ncd=dmn_trv;
        }

        /* There are coordinates; one must be chosen 
        Scope definition: In the same group of the variable or beneath (closer to root) 
        Above: out of scope */
        else if(dmn_trv->crd_nbr > 0) {

          crd_sct *crd=NULL; /* [sct] Coordinate to assign to dimension of variable */

          /* Choose the "in scope" coordinate for the variable and assign it to the variable dimension */
          crd=nco_scp_var_crd(&var_trv,dmn_trv);

          /* The "in scope" coordinate is returned */
          if (crd) {
            if(dbg_lvl_get() == 13){ 
              (void)fprintf(stdout,"%s: INFO %s reports dimension [%d]%s of variable <%s> in scope of coordinate <%s>\n",prg_nm_get(),fnc_nm, 
                dmn_idx_var,var_trv.var_dmn[dmn_idx_var].dmn_nm_fll,var_trv.nm_fll,crd->crd_nm_fll);         
            } /* endif dbg */ 

            /* Mark as True */
            trv_tbl->lst[var_idx].var_dmn[dmn_idx_var].is_crd_var=True;

            /* Store coordinate */
            trv_tbl->lst[var_idx].var_dmn[dmn_idx_var].crd=crd;

          /* None was found in scope */
          }else {
            if(dbg_lvl_get() == 13){ 
              (void)fprintf(stdout,"%s: INFO %s reports dimension [%d]%s of variable <%s> with out of scope coordinate\n",prg_nm_get(),fnc_nm, 
                dmn_idx_var,var_trv.var_dmn[dmn_idx_var].dmn_nm_fll,var_trv.nm_fll);         
            } /* endif dbg */

            /* Mark as False */
            trv_tbl->lst[var_idx].var_dmn[dmn_idx_var].is_crd_var=False;

            /* Store the unique dimension as if it was a non coordinate */
            trv_tbl->lst[var_idx].var_dmn[dmn_idx_var].ncd=dmn_trv;

          } /* None was found in scope */
        } /* There are coordinates; one must be chosen */
      } /* Loop dimensions for object (variable)  */
    } /* Filter variables  */
  } /* Loop table */



  /* Check if bool array is all filled  */

#ifdef NCO_SANITY_CHECK
  /* Loop table */
  for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){

    /* Filter variables  */
    if(trv_tbl->lst[var_idx].nco_typ == nco_obj_typ_var){
      trv_sct var_trv=trv_tbl->lst[var_idx];   
      /* Loop dimensions for object (variable)  */
      for(int dmn_idx_var=0;dmn_idx_var<var_trv.nbr_dmn;dmn_idx_var++) {
        if(trv_tbl->lst[var_idx].var_dmn[dmn_idx_var].is_crd_var == nco_obj_typ_err) {

          if(dbg_lvl_get() == 13 ){
            (void)fprintf(stdout,"%s: OOPSY %s reports variable <%s> with NOT filled dimension [%d]%s\n",prg_nm_get(),fnc_nm,
              var_trv.nm_fll,dmn_idx_var,var_trv.var_dmn[dmn_idx_var].dmn_nm_fll);        
          } /* endif dbg */
        }
      } /* Loop dimensions for object (variable)  */
    } /* Filter variables  */
  } /* Loop table */
#endif /* NCO_SANITY_CHECK */


  /* Check if bool array is all filled  */

#ifdef NCO_SANITY_CHECK
  /* Loop table */
  for(unsigned var_idx=0;var_idx<trv_tbl->nbr;var_idx++){

    /* Filter variables  */
    if(trv_tbl->lst[var_idx].nco_typ == nco_obj_typ_var){
      trv_sct var_trv=trv_tbl->lst[var_idx];   
      /* Loop dimensions for object (variable)  */
      for(int dmn_idx_var=0;dmn_idx_var<var_trv.nbr_dmn;dmn_idx_var++) {
        assert(trv_tbl->lst[var_idx].var_dmn[dmn_idx_var].is_crd_var != nco_obj_typ_err);
      } /* Loop dimensions for object (variable)  */
    } /* Filter variables  */
  } /* Loop table */
#endif /* NCO_SANITY_CHECK */

} /* nco_bld_var_dmn() */



void                          
nco_wrt_trv_tbl                      /* [fnc] Obtain file information from GTT (Group Traversal Table) for debugging  */
(const int nc_id,                    /* I [ID] File ID */
 const trv_tbl_sct * const trv_tbl,  /* I [sct] GTT (Group Traversal Table) */
 nco_bool use_flg_xtr)               /* I [flg] Use flg_xtr in selection */
{

  const char fnc_nm[]="nco_wrt_trv_tbl()"; /* [sng] Function name  */

  int nbr_dmn_var;             /* [nbr] Number of variables in group */
  int grp_id;                  /* [id] Group ID */
  int var_id;                  /* [id] Variable ID */
  int dmn_id_var[NC_MAX_DIMS]; /* [id] Dimensions IDs array for variable */

  for(unsigned idx_var=0;idx_var<trv_tbl->nbr;idx_var++){
    trv_sct var_trv=trv_tbl->lst[idx_var];

    nco_bool flg_xtr;
    
    if (use_flg_xtr)flg_xtr=var_trv.flg_xtr; else flg_xtr=True;

    /* If object is an extracted variable... */ 
    if(var_trv.nco_typ == nco_obj_typ_var && flg_xtr){

      if(dbg_lvl_get() == 14){
        (void)fprintf(stdout,"%s: INFO %s variable <%s>",prg_nm_get(),fnc_nm,var_trv.nm_fll);        
      } /* endif dbg */


      /* Obtain group ID where variable is located using full group name */
      (void)nco_inq_grp_full_ncid(nc_id,var_trv.grp_nm_fll,&grp_id);

      /* Obtain variable ID */
      (void)nco_inq_varid(grp_id,var_trv.nm,&var_id);

      /* Get type of variable and number of dimensions */
      (void)nco_inq_var(grp_id,var_id,(char *)NULL,(nc_type *)NULL,&nbr_dmn_var,(int *)NULL,(int *)NULL);

      /* Get dimension IDs for variable */
      (void)nco_inq_vardimid(grp_id,var_id,dmn_id_var);

      if(dbg_lvl_get() == 14){
        (void)fprintf(stdout," %d dimensions: ",nbr_dmn_var);        
      } /* endif dbg */

      /* Variable dimensions */
      for(int dmn_idx_var=0;dmn_idx_var<nbr_dmn_var;dmn_idx_var++){

        char dmn_nm_var[NC_MAX_NAME+1]; /* [sng] Dimension name */
        long dmn_sz_var;                /* [nbr] Dimension size */ 

        /* Get dimension name */
        (void)nco_inq_dim(grp_id,dmn_id_var[dmn_idx_var],dmn_nm_var,&dmn_sz_var);

        if(dbg_lvl_get() == 14){
          (void)fprintf(stdout,"#%d'%s' ",
            dmn_id_var[dmn_idx_var],dmn_nm_var);
        }
      }

      if(dbg_lvl_get() == 14){
        (void)fprintf(stdout,"\n");
      }

    } /* endif */

  } /* end loop over uidx */

}
