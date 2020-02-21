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
 * Deberás haber recibido una copia de la Licencia Pública General
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

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <stddef.h>
#include <strings.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include "gawkapi.h"

static const gawk_api_t *api;   /* Conveniencia para usar macros */
static awk_ext_id_t ext_id;
static const char *ext_version = "extensión conector: versión 1.0";

static awk_bool_t (*init_func)(void) = NULL;

int plugin_is_GPL_compatible;

#define PENDIENTES 100

static int
set_non_blocking(int fd)
{
    int flags;

    if (((flags = fcntl(fd, F_GETFL)) == -1)
        || (fcntl(fd, F_SETFL, (flags|O_NONBLOCK)) == -1)) {
        update_ERRNO_int(errno);
        return -1;
    }
    return 0;
}

/* haz_trae_es -- Trae descriptores E/S de un fichero o flujo */

static awk_value_t *
haz_trae_es(int nargs, awk_value_t *resultado, struct awk_ext_func *unused)
{
    awk_array_t lista_es;
    awk_value_t lista_arg, nombre, valor;
    awk_value_t fichero, tipo;
    const awk_input_buf_t *entrada;
    const awk_output_buf_t *salida;

    assert(resultado != NULL);

    /* Sólo acepta 2 argumentos */
    if (nargs < 2 || nargs > 3) {
        lintwarn(ext_id, "conector: nº de argumentos incorrecto");
        return make_number(-1, resultado);
    }

    /* Trae nombre fichero y tipo */  
    if (   ! get_argument(0, AWK_STRING, &fichero)
        || ! get_argument(1, AWK_STRING, &tipo))
    {   
        lintwarn(ext_id, "conector: tipo de argumento incorrecto");
        return make_number(-1, resultado);
    }

    /* Almacena en lista EntSal, o en 3 argumento, los descriptores de E/S */
    if (nargs == 3) {
        if (! get_argument(2, AWK_ARRAY, &lista_arg)) {
            warning(ext_id, "conector: el argumento E/S debe ser una lista");
            update_ERRNO_string("conector: argumento E/S incorrecto");
            return make_number(-1, resultado);
        }
        clear_array(lista_arg.array_cookie);
        lista_es = lista_arg.array_cookie;
    } else {
        lista_es = create_array();
        valor.val_type = AWK_ARRAY;
        valor.array_cookie = lista_es;

        if (! sym_update("EntSal", &valor))
            lintwarn(ext_id, "conector: error creando símbolo \"EntSal\"");
        lista_es = valor.array_cookie;
    }

    /* Por ahora, en caso de error, reintentar cada 0.6s */
    while (! get_file(fichero.str_value.str, fichero.str_value.len,
                        tipo.str_value.str, -1, &entrada, &salida))
         nanosleep((const struct timespec[]){{0, 600000000L}}, NULL);

    if (entrada == NULL || salida == NULL) {
        update_ERRNO_int(EIO);
        update_ERRNO_string("conector: error de E/S");
        lintwarn(ext_id, "conector: error trayendo descriptores de E/S");
        return make_number(-1, resultado);
    }

    /* EntSal["entrada"] = descriptor Entrada */
    (void) make_const_string("entrada", 7, &nombre);
    (void) make_number(entrada->fd, &valor);
    if (! set_array_element(lista_es, &nombre, &valor)) {
        lintwarn(ext_id, "conector: fallo al crear elemento entrada");
        return make_number(-1, resultado);
    }

    /* EntSal["salida"] = descriptor Salida */
    (void) make_const_string("salida",  6, &nombre);
    (void) make_number(fileno(salida->fp), &valor);
    if (! set_array_element(lista_es, &nombre, &valor)) {
        lintwarn(ext_id, "conector: fallo al crear elemento salida");
        return make_number(-1, resultado);
    }

    return make_number(1, resultado);
}

/* haz_crea_toma -- Crea toma de escucha */

static awk_value_t *
haz_crea_toma(int nargs, awk_value_t *resultado,
            struct awk_ext_func *unused)
{
    int df_cnx_ent;
    awk_value_t puerto;
    struct sockaddr_in servidor;

    /* Sólo acepta 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "conector: nº de argumentos incorrecto");
        return make_number(-1, resultado);
    }

    if (! get_argument(0, AWK_NUMBER, &puerto)) {   
        lintwarn(ext_id, "conector: tipo de argumento incorrecto");
        return make_number(-1, resultado);
    }

    /* Crea toma de entrada */
    df_cnx_ent = socket(AF_INET, SOCK_STREAM, 0);

    if (df_cnx_ent < 0) {
        lintwarn(ext_id, "conector: error creando toma de entrada");
        return make_number(-1, resultado);
    }

    //set_non_blocking(df_cnx_ent);

    /* Asocia toma de entrada a una dirección IP y un puerto */
    bzero(&servidor, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons((int) puerto.num_value);

    if (bind(df_cnx_ent, (struct sockaddr*) &servidor,
            sizeof(servidor)) < 0) {
        lintwarn(ext_id, "conector: error enlazando toma de entrada");
        return make_number(-1, resultado);
    }

    /* Poner toma en modo escucha */
    if (listen(df_cnx_ent, PENDIENTES) < 0){
        lintwarn(ext_id, "conector: error poniendo toma en escucha");
        return make_number(-1, resultado);
    }

    return make_number(df_cnx_ent, resultado);
}

/* haz_escucha -- Extrae la primera conexión de la cola de conexiones */

static awk_value_t *
haz_escucha(int nargs, awk_value_t *resultado,
            struct awk_ext_func *unused)
{
    int df_cnx_sal;
    awk_value_t toma_ent;
    struct sockaddr_in cliente;
    socklen_t lnt = (socklen_t) sizeof(cliente);

    /* Sólo acepta 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "conector: nº de argumentos incorrecto");
        return make_number(-2, resultado);
    }

    if (! get_argument(0, AWK_NUMBER, &toma_ent)) {
        lintwarn(ext_id, "conector: tipo de argumento incorrecto");
        return make_number(-3, resultado);
    }

    /* Extrae la primera conexión de la cola de conexiones */
    df_cnx_sal = accept((int) toma_ent.num_value, 
            (struct sockaddr*) &cliente, &lnt);
    perror("accpet()");
    /* Espera conexiones pero no envia nada. Cerrar canal con cliente */
    //close(df_cnx_sal);

    return make_number(df_cnx_sal, resultado);
}

static awk_ext_func_t lista_de_funciones[] = {
    { "trae_es",   haz_trae_es,   0, 0, awk_false, NULL },
    { "crea_toma", haz_crea_toma, 0, 0, awk_false, NULL },
    { "escucha",   haz_escucha,   0, 0, awk_false, NULL },
};

/* Define función de carga */

dl_load_func(lista_de_funciones, conector, "")
