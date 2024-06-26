#ifndef _SYM_H_
#define _SYM_H_

extern void     illname        (void);
extern void     multidef       (char *sname);
extern int      needsub        (void);
extern int      symname        (char *sname);
extern int      getlabel       (void);
extern int      find_local     (char *sname);
extern SYMBOL * find_global    (char *sname);
extern int      add_local      (char *sname, int identity, int type, int offset, int storage_class);
extern SYMBOL * add_global     (char *sname, int identity, int type, int offset, int storage_class);
extern void     declare_local  (int typ, int stclass, int otag);
extern void     declare_global (int type, int storage, TAG_SYMBOL * mtag, int otag, int is_struct);
extern void     struct_init    (TAG_SYMBOL * tag, char *sname);
extern int      init           (char *sname, int type, int identity, int *dim, TAG_SYMBOL * tag);
extern int      initials       (char *sname, int type, int identity, int dim, int otag);

#endif // #ifndef _SYM_H_
