/*
 * Autor: Ulpiano Tur de Vargas <ulpiano.tur.devargas@gmail.com>
 *
 * Este programa es software libre; puedes distribuirlo y/o
 * modificarlo bajo los términos de la Licencia Pública General de GNU
 * según la publicó la Fundación del Software Libre; ya sea la versión 3, o
 * (a su elección) una versión superior.
 *
 * Este programa se distribuye con la esperanza de que sea útil,
 * pero SIN NINGUNA GARANTIA; ni siquiera la garantía implícita de
 * COMERCIABILIDAD o APTITUD PARA UN PROPÓSITO DETERMINADO. Vea la
 * Licencia Pública General de GNU para más detalles.
 *
 * Deberías haber recibido una copia de la Licencia Pública General
 * de GNU junto con este software; mira el fichero LICENSE. Si
 * no, mira <https://www.gnu.org/licenses/>.
 *
 * Author: Ulpiano Tur de Vargas <ulpiano.tur.devargas@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this software; see the file LICENSE. If
 * not, see <https://www.gnu.org/licenses/>.
 */

//#ifdef HAVE_CONFIG_H
//#include <config.h>
//#endif

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gawkapi.h"

#define _GNU_SOURCE

static const gawk_api_t *api;   /* Conveniencia para usar macros */
static awk_ext_id_t ext_id;
static const char *ext_version = "extensión ramal: versión 1.0";
static awk_bool_t (*init_func)(void) = NULL;

int plugin_is_GPL_compatible;


/* pon_num_en_coleccion -- añadir elemento numérico a la colección */

static void
pon_num_en_coleccion(awk_array_t coleccion, const char *sub, double num)
{
    awk_value_t index, value;

    set_array_element(coleccion,
        make_const_string(sub, strlen(sub), & index),
        make_number(num, & value));

}

/* haz_nuevo_ramal -- proporciona clone3 a gawk cargándola dinámicamente */

static awk_value_t *
haz_nuevo_ramal(int nargs, awk_value_t *resultado, struct awk_ext_func *desusado)
{
    int pid = -1;
    struct clone_args args;

    (void) nargs;
    (void) desusado;
    assert(resultado != NULL);

    args.flags       = CLONE_FS|CLONE_IO;
    /* No se envía señal al padre cuando el hijo termina */
    args.exit_signal = 0;
    /* Padre e hijo comparten el mismo área de pila */
    args.stack       = NULL;
    args.stack_size  = 0;

    /* Nuevo ramal de ejecución */
    pid = clone3(&args, sizeof(args));

    if (pid < 0)
        update_ERRNO_int(errno);
    else if (pid == 0) {
        /* Actualiza PROCINFO en el ramal del hijo, si existe la colección */
        awk_value_t procinfo;

        if (sym_lookup("PROCINFO", AWK_ARRAY, &procinfo)) {
            if (procinfo.val_type != AWK_ARRAY) {
                if (do_lint)
                    lintwarn(ext_id, "ramal: PROCINFO no es una colección!");
            } else {
                pon_num_en_coleccion(procinfo.array_cookie, "pid", getpid());
                pon_num_en_coleccion(procinfo.array_cookie, "ppid", getppid());
            }
        }
    }

    /* Valor de retorno */
    return make_number(pid, resultado);
}

/* haz_esperapid -- proporciona waitpid() a gawk cargándola dinámicamente */

static awk_value_t *
haz_esperapid(int nargs, awk_value_t *resultado, struct awk_ext_func *desusado)
{
    awk_value_t pid;
    int ret = -1;
    int opciones = 0;

    (void) nargs;
    (void) desusado;
    assert(resultado != NULL);

    if (get_argument(0, AWK_NUMBER, &pid)) {
        opciones = WNOHANG|WUNTRACED;
        ret = waitpid(pid.num_value, NULL, opciones);
        if (ret < 0)
            update_ERRNO_int(errno);
    }

    /* Valor de retorno */
    return make_number(ret, resultado);
}


/* haz_espera -- proporciona wait() a gawk cargándola dinámicamente */

static awk_value_t *
haz_espera(int nargs, awk_value_t *resultado, struct awk_ext_func *desusado)
{
    int ret;

    (void) nargs;
    (void) desusado;
    assert(resultado != NULL);

    ret = wait(NULL);
    if (ret < 0)
        update_ERRNO_int(errno);

   /* Valor de retorno */
    return make_number(ret, resultado);
}

static awk_ext_func_t lista_de_funciones[] = {
    { "ramal",     haz_nuevo_ramal, 0, 0, awk_false, NULL },
    { "esperapid", haz_esperapid,   1, 1, awk_false, NULL },
    { "espera",    haz_espera,      0, 0, awk_false, NULL },
};

/* Cargar funciones */

dl_load_func(lista_de_funciones, ramal, "")