/* Various routines or globals called/referenced from OWC soft float library */
#include <assert.h>
#include <errno.h>

/* floating point globals - FIXME should initialize at startup? */
char _8087;
char _real87;
char _chipbug;

/*
    __FPE_exception is called from machine language with parm in AX
    (see "fstatus" module in CGSUPP)
*/
void __FPE_exception( int fpe_type )
{
    //_RWD_FPE_handler( fpe_type );
    assert(0);      /* FIXME abort for now */
}

void __set_ERANGE( void )
{
    errno = ERANGE;
}

