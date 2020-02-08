/*
 * Autor: Ulpiano Tur de Vargas <ulpiano.tur.devargas@gmail.com>
 *
 * Este programa es software libre; puedes distribuirlo y/o
 * modificarlo bajo los t�rminos de la Licencia P�blica General de GNU
 * seg�n la public� la Fundaci�n del Software Libre; ya sea la versi�n 3, o
 * (a su elecci�n) una versi�n superior.
 *
 * Este programa se distribuye con la esperanza de que sea �til,
 * pero SIN NINGUNA GARANTIA; ni siquiera la garant�a impl�cita de
 * COMERCIABILIDAD o APTITUD PARA UN PROP�SITO DETERMINADO. Vea la
 * Licencia P�blica General de GNU para m�s detalles.
 *
 * Deber�s haber recibido una copia de la Licencia P�blica General
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

#define API_AWK_V2 1

#define TIEMPOMUERTO 5

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <poll.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/select.h>

#include "gawkapi.h"

static const gawk_api_t *api;	/* Conveniencia para usar macros */
static awk_ext_id_t ext_id;
static const char *ext_version = "extensi�n sereno E/S: versi�n 1.0";

static awk_bool_t (*init_func)(void) = NULL;

int plugin_is_GPL_compatible;

/* haz_trae_es -- Trae descriptores E/S de un fichero o flujo */

#ifdef API_AWK_V2
static awk_value_t *
haz_trae_es(int nargs, awk_value_t *resultado, struct awk_ext_func *unused)
#else
static awk_value_t *
haz_trae_es(int nargs, awk_value_t *resultado)
#endif
{
    awk_array_t lista_es;
    awk_value_t lista_arg, nombre, valor;
    awk_value_t fichero, tipo;
    const awk_input_buf_t *entrada;
    const awk_output_buf_t *salida;

    assert(resultado != NULL);

    /* S�lo acepta 2 argumentos */
    if (nargs < 2 || nargs > 3) {
        lintwarn(ext_id, "trae_es: n� e argumentos incorrecto");
        return make_number(-1, resultado);
    }

    /* Trae nombre fichero y tipo */  
    if (   ! get_argument(0, AWK_STRING, &fichero)
        || ! get_argument(1, AWK_STRING, &tipo))
    {   
        lintwarn(ext_id, "trae_es: tipo de argumento incorrecto");
        return make_number(-1, resultado);
    }

    /* Almacena en lista EntSal, o en 3 argumento, los descriptores de E/S */
    if (nargs == 3) {
        if (! get_argument(2, AWK_ARRAY, &lista_arg)) {
            warning(ext_id, "trae_es: el argumento E/S debe ser una lista");
            update_ERRNO_string("trae_es: argumento E/S incorrecto");
            return make_number(-1, resultado);
        }
        clear_array(lista_arg.array_cookie);
        lista_es = lista_arg.array_cookie;
    } else {
        lista_es = create_array();
        valor.val_type = AWK_ARRAY;
        valor.array_cookie = lista_es;

        if (! sym_update("EntSal", &valor))
            lintwarn(ext_id, "trae_es: sym_update(\"EntSal\") no se crea!");
        lista_es = valor.array_cookie;
    }
    
    /* En caso de error se intenta nevamente a los 0.6 s  */
    while (! get_file(fichero.str_value.str, fichero.str_value.len,
                        tipo.str_value.str, -1, &entrada, &salida))
         nanosleep((const struct timespec[]){{0, 600000000L}}, NULL);

    if (entrada == NULL || salida == NULL) {
        update_ERRNO_int(EIO);
        update_ERRNO_string("trae_es: error de E/S");
        lintwarn(ext_id, "trae_es: error trayendo descriptores de E/S");
        return make_number(-1, resultado);
    }

    /* EntSal["entrada"] = descriptor Entrada */
    (void) make_const_string("entrada", 7, &nombre);
    (void) make_number(entrada->fd, &valor);
    if (! set_array_element(lista_es, &nombre, &valor)) {
        lintwarn(ext_id, "trae_es: fallo al crear elemento entrada");
        return make_number(-1, resultado);
    }

    /* EntSal["salida"] = descriptor Salida */
    (void) make_const_string("salida",  6, &nombre);
    (void) make_number(fileno(salida->fp), &valor);
    if (! set_array_element(lista_es, &nombre, &valor)) {
        lintwarn(ext_id, "trae_es: fallo al crear elemento salida");
        return make_number(-1, resultado);
    }

    return make_number(1, resultado);
}

/* haz_sondea  -- Proporciona funci�n pol() cargada din�micamente */

#ifdef API_AWK_V2
static awk_value_t *
haz_sondea(int nargs, awk_value_t *resultado, struct awk_ext_func *unused)
#else
static awk_value_t *
haz_sondea(int nargs, awk_value_t *resultado)
#endif
{
    int ret;
    struct pollfd dsf[2];
    awk_value_t lista_es, nombre, valor;

    assert(resultado != NULL);

    /* Sólo acept 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "sondea: n� e argumentos incorrecto");
        return make_number(-1, resultado);
    }

    if (! get_argument(0, AWK_ARRAY, &lista_es)) {
        warning(ext_id, "sondea: el argumento E/S debe ser una lista");
        update_ERRNO_string("sondea: argumento E/S incorrecto");
        return make_number(-1, resultado);
    }
    
    /* Entrada: averigua si es posible leer */
    (void) make_const_string("entrada", 7, &nombre);
    get_array_element(lista_es.array_cookie, &nombre, AWK_NUMBER, &valor);
    dsf[0].fd = valor.num_value;
    dsf[0].events = POLLIN;

    /* Salida: averigua si es posible escribir */
    (void) make_const_string("salida", 6, &nombre);
    get_array_element(lista_es.array_cookie, &nombre, AWK_NUMBER, &valor);
    dsf[1].fd = valor.num_value;
    dsf[1].events = POLLOUT;

    while ((ret = poll(dsf, 2, TIEMPOMUERTO * 1000)) == 0) {
            ;
    }

    /* Devuelve lo mismo que poll() */
    return make_number(ret, resultado);
}

/* haz_vigila -- Proporciona funci�n select() cargada din�micamente */

#ifdef API_AWK_V2
static awk_value_t *
haz_vigila(int nargs, awk_value_t *resultado, struct awk_ext_func *unused)
#else
static awk_value_t *
haz_vigila(int nargs, awk_value_t *resultado)
#endif
{
    int ret, df_lect, df_escr;
    fd_set dsc_lect;
    fd_set dsc_escr;
    struct timeval tmp;
    awk_value_t lista_es, nombre, valor;

    assert(resultado != NULL);

    /* S�lo acept 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "vigila: n� de argumentos incorrecto");
        return make_number(-1, resultado);
    }

    if (! get_argument(0, AWK_ARRAY, &lista_es)) {
        warning(ext_id, "vigila: el argumento E/S debe ser una lista");
        update_ERRNO_string("vigila: argumento E/S incorrecto");
        return make_number(-1, resultado);
    }

    /* Limpia colección de descriptores E/S */
    FD_ZERO(&dsc_lect);
    FD_ZERO(&dsc_escr);

    /* Entrada: monitoriza si es posible leer */
    (void) make_const_string("entrada", 7, &nombre);
    get_array_element(lista_es.array_cookie, &nombre, AWK_NUMBER, &valor);
    df_lect = valor.num_value;
    FD_SET(df_lect, &dsc_lect);

    /* Salida: monitoriza si esposible escribir */
    (void) make_const_string("salida", 6, &nombre);
    get_array_element(lista_es.array_cookie, &nombre, AWK_NUMBER, &valor);
    df_escr = valor.num_value;
    FD_SET(df_escr, &dsc_escr);

    /* Espera 5 segundos por ahora */
    tmp.tv_sec = TIEMPOMUERTO;
    tmp.tv_usec = 0;

    while (1) {
        ret = select(FD_SETSIZE, &dsc_lect, &dsc_escr, NULL, &tmp);
        return make_number(ret, resultado);
        /* �Listo para leer y escribir? */
        if (ret 
            && FD_ISSET(df_lect, &dsc_lect)
            && FD_ISSET(df_escr, &dsc_escr)) 
        {
            break;  
        }
    }

    /* Devuelve lo mismo que select() */
    return make_number(ret, resultado);
}

#ifdef API_AWK_V2
static awk_ext_func_t lista_de_funciones[] = {
    { "trae_es", haz_trae_es, 0, 0, awk_false, NULL },
    { "sondea",  haz_sondea,  0, 0, awk_false, NULL },
    { "vigila",  haz_vigila,  0, 0, awk_false, NULL },
};
#else
static awk_ext_func_t lista_de_funciones[] = {
    { "trae_es", haz_trae_es, 0 },
    { "sondea",  haz_sondea,  0 },
    { "vigila",  haz_vigila, 0  },
};
#endif

/* Define funci�n de carga */

dl_load_func(lista_de_funciones, sereno, "")
