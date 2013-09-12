/******************************************************************************
*	COPYRIGHT (C) STMicroelectronics 1998
*
*   File name   : blt_surf.h
*   Description : surface orientated testing macros
*
*	Date          Modification                                    Initials
*	----          ------------                                    --------
*   12 Apr 2002   Creation                                        GS
*
******************************************************************************/
extern BOOL surf_create          (parse_t *pars_p, char *result_sym_p);
extern BOOL surf_delete          (parse_t *pars_p, char *result_sym_p);
extern BOOL surf_check_inside    (parse_t *pars_p, char *result_sym_p);
extern BOOL surf_check_outside   (parse_t *pars_p, char *result_sym_p);
extern BOOL surf_check_shape     (parse_t *pars_p, char *result_sym_p);
extern BOOL surf_fill            (parse_t *pars_p, char *result_sym_p);

extern BOOL surf_STBLIT_FillRect (parse_t *pars_p, char *result_sym_p);
extern BOOL surf_STBLIT_CopyRect (parse_t *pars_p, char *result_sym_p);
extern BOOL surf_STBLIT_XYL      (parse_t *pars_p, char *result_sym_p);


extern BOOL surf_benchmark       (parse_t *pars_p, char *result_sym_p);


