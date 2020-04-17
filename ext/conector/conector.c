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

#define API_AWK_V2

#define _GNU_SOURCE

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <arpa/inet.h>

#include "gawkapi.h"

#include "cntr_defcom.h"
#include "cntr_ruta.h"
#include "cntr_toma.h"
#include "cntr_stoma.h"

#define MAX_EVENTOS        10

static const gawk_api_t *api; /* Conveniencia para usar macros */
static awk_ext_id_t ext_id;
static const char *ext_version = "extensión conector: versión 1.0";

static awk_bool_t inicia_conector(void);
static awk_bool_t (*init_func)(void) = inicia_conector;

int plugin_is_GPL_compatible;

/* Variables globales que mantienen su valor */

static t_cntr_ruta rt;  /* Ruta de conexión */

static int libera; /* Indica memoria liberada */

/* pon_num_en_coleccion -- Añadir elemento numérico a la colección */

static void
pon_num_en_coleccion(awk_array_t coleccion, const char *sub, double num)
{
    awk_value_t index, value;

    set_array_element(coleccion,
        make_const_string(sub, strlen(sub), &index),
        make_number(num, &value));
}

/* pon_txt_en_coleccion -- Añadir elemento textual a la colección */

static void
pon_txt_en_coleccion(awk_array_t coleccion, const char *sub, const char *txt)
{
    awk_value_t index, value;

    set_array_element(coleccion,
        make_const_string(sub, strlen(sub), &index),
        make_const_string(txt, strlen(txt), &value));
}

/**
 * Funciones que proporciona 'conector.c' a GAWK
 */

/* haz_crea_toma -- Crea toma de escucha */

static awk_value_t *
#ifdef API_AWK_V2
haz_crea_toma(int nargs, awk_value_t *resultado, struct awk_ext_func *desusado)
#else
haz_crea_toma(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t nombre;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs != 1)
        fatal(ext_id, "creatoma: nº de argumentos incorrecto");

    if (! get_argument(0, AWK_STRING, &nombre))
        fatal(ext_id, "creatoma: tipo de argumento incorrecto");

    if (   nombre.str_value.str == NULL
        || cntr_nueva_ruta((const char *) nombre.str_value.str, &rt) < 0
        || !rt.local)
    {
        fatal(ext_id, "creatoma: fichero de red remoto o no válido");
    }

    if (cntr_nueva_toma_servidor(&rt) == CNTR_ERROR)
        fatal(ext_id, "creatoma: error creando toma de escucha");

    return make_number(rt.toma->servidor, resultado);
}

/* haz_cierra_toma -- Cierra toma de escucha */

static awk_value_t *
#ifdef API_AWK_V2
haz_cierra_toma(int nargs, awk_value_t *resultado, struct awk_ext_func *desusado)
#else
haz_cierra_toma(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t nombre;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "cierratoma: nº de argumentos incorrecto");
        return make_number(-1, resultado);
    }

    if (! get_argument(0, AWK_STRING, &nombre)) {   
        lintwarn(ext_id, "cierratoma: tipo de argumento incorrecto");
        return make_number(-1, resultado);
    }

    if (strcmp((const char *) nombre.str_value.str, rt.nombre) != 0) {
        lintwarn(ext_id, "cierratoma: toma no válida");
        return make_number(-1, resultado);
    }

    t_elector_es opcn;
    opcn.es_servidor = 1;
    opcn.es_cliente  = 0;
    if (cntr_cierra_toma(&rt, opcn) == CNTR_ERROR)
        lintwarn(ext_id, "cierratoma: error cerrando toma");

    return make_number(0, resultado);
}

/* haz_extrae_primera -- Extrae primera conexión de la cola de conexiones */

static awk_value_t *
#ifdef API_AWK_V2
haz_extrae_primera(int nargs, awk_value_t *resultado,
                   struct awk_ext_func *desusado)
#else
haz_extrae_primera(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t valorarg;
    struct sockaddr_in cliente;
    socklen_t lnt = (socklen_t) sizeof(cliente);

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs < 1 || nargs > 2)
        fatal(ext_id, "traepctoma: nº de argumentos incorrecto");

    if (! get_argument(0, AWK_STRING, &valorarg))
        fatal(ext_id, "traepctoma: tipo de argumento incorrecto");

    if (strcmp((const char *) valorarg.str_value.str, rt.nombre) != 0)
        fatal(ext_id, "traepctoma: toma escucha incorrecta");

    fd_set lst_df_sondear_lect, lst_df_sondear_escr;

    /* Borrar colección de tomas E/S a sondear */
    FD_ZERO(&lst_df_sondear_lect);
    FD_ZERO(&lst_df_sondear_escr);
    /* Sondear toma de escucha */
    FD_SET(rt.toma->servidor, &lst_df_sondear_lect);

    while (1) {
        /* Parar hasta que llegue evento a una o más tomas activas */
        if (select(FD_SETSIZE, &lst_df_sondear_lect, &lst_df_sondear_escr,
                   NULL, NULL) < 0) {
            perror("select");
            fatal(ext_id, "traepctoma: error esperando eventos E/S");
        }
        /* Atender tomas con eventos de entrada pendientes */
        if (FD_ISSET(rt.toma->servidor, &lst_df_sondear_lect)) {
            /* Extraer primera conexión de la cola de conexiones */
            rt.toma->cliente = accept(rt.toma->servidor,
                                    (struct sockaddr*) &cliente,
                                    &lnt);
            /* ¿Es cliente? */
            if (rt.toma->cliente < 0) {
                perror("accept");
                fatal(ext_id, "traepctoma: error al extraer conexión");
            }
            
            /* Sí; es cliente */

            /* Anunciar cliente */
            if (nargs == 2) {
                if (get_argument(1, AWK_ARRAY, &valorarg)) {
llena_coleccion:
                    pon_txt_en_coleccion(valorarg.array_cookie, "dir",
                                         inet_ntoa(cliente.sin_addr));
                    pon_num_en_coleccion(valorarg.array_cookie, "pto",
                                         (double)ntohs(cliente.sin_port));
                } else {
                    if (valorarg.val_type == AWK_UNDEFINED) {
                        set_argument(1, create_array());
                        goto llena_coleccion;
                    } else {
                        lintwarn(ext_id,
                                 "traepctoma: segundo argumento incorrecto");
                    }
                }
            }
sondea_salida:
            FD_ZERO(&lst_df_sondear_lect);
            FD_ZERO(&lst_df_sondear_escr);
            FD_SET(rt.toma->cliente, &lst_df_sondear_lect);
            FD_SET(rt.toma->cliente, &lst_df_sondear_escr);
        } else {
            if (   FD_ISSET(rt.toma->cliente, &lst_df_sondear_lect)
                && FD_ISSET(rt.toma->cliente, &lst_df_sondear_escr))
                break;
            else
                goto sondea_salida;
        }
    }

    /* Devuelve accept(), es decir, el descriptor de fichero de la 
       toma de conexión recién creada con el cliente. */
    return make_number(rt.toma->cliente, resultado);
}

/**
 * Procesador bidireccional que proporciona 'conector' a GAWK
 */

/* Los datos del puntero oculto */

typedef struct datos_proc_bidireccional {
    char    *tope;  /* Tope de datos                              */
    char    *sdrt;  /* Separador de registro. Variable RS de gawk */
    size_t  tsr;    /* Tamaño cadena separador de registro        */
    size_t  max;    /* Tamaño máximo asignado al tope             */
    ssize_t lgtope; /* Tamaño actual del tope                     */
    size_t  lgtreg; /* Tamaño actual del registro                 */
    int     ptrreg; /* Puntero inicio registro en tope (actual)   */
    int     ptareg; /* Puntero inicio registro en tope (anterior) */
} t_datos_conector;

/* libera_conector -- Libera datos */

static void
libera_conector(t_datos_conector *flujo)
{
    /* Evita liberar memoria dos veces */
    if (libera > 0)
        libera--;
    else
        return;

    gawk_free(flujo->sdrt);
    gawk_free(flujo->tope);
    gawk_free(flujo);
}

/* long_registro -- Calcula distancia entre dos posiciones de memoria */

static int
long_registro(char *ini, char *fin)
{
    int c = 0;
    char *i = ini;

    while (i++ != fin)
        c++;

    return c;
}

/* cierra_toma_entrada -- Cerrar toma conexión entrada */

static void
cierra_toma_entrada(awk_input_buf_t *iobuf)
{
    t_datos_conector *flujo;

    if (iobuf == NULL || iobuf->opaque == NULL)
        return;

    flujo = (t_datos_conector *) iobuf->opaque;
    libera_conector(flujo);

    /* haz_cierra_toma() hace esto con la toma de escucha */

    iobuf->fd = INVALID_HANDLE; /* Por ahora es un df ficticio */
}

/* cierra_toma_salida -- Cerrar toma conexión salida */

static int
cierra_toma_salida(FILE *fp, void *opaque)
{
    t_datos_conector *flujo;

    if (opaque == NULL)
        return EOF;

    flujo = (t_datos_conector *) opaque;

    libera_conector(flujo);

    t_elector_es opcn;
    opcn.es_servidor = 0;
    opcn.es_cliente  = 1;
    opcn.forzar      = 1;
    if (cntr_cierra_toma(&rt, opcn) == CNTR_ERROR)
        lintwarn(ext_id, "conector: error cerrando toma cliente");

    /* Se usa la toma_salida en su lugar, pero hay que cerrarlo */
    fclose(fp);

    return 0;
}

/* limpia_toma_salida -- Apagar toma de salida */

static int
limpia_toma_salida(FILE *fp, void *opaque)
{
    (void) fp;
    (void) opaque;

    return 0;
}

/* maneja_error -- De momento no hacer nada */

static int
maneja_error(FILE *fp, void *opaque)
{
    (void) fp;
    (void) opaque;

    return 0;
}

/* conector_trae_registro -- Lee un registro cada vez */

static int
#ifdef API_AWK_V2
conector_trae_registro(char **out, awk_input_buf_t *iobuf, int *errcode,
                       char **rt_start, size_t *rt_len,
                       const awk_fieldwidth_info_t **desusado)
#else
conector_trae_registro(char **out, awk_input_buf_t *iobuf, int *errcode,
                       char **rt_start, size_t *rt_len)
#endif
{
    t_datos_conector *flujo;

    (void) errcode;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    if (out == NULL || iobuf == NULL || iobuf->opaque == NULL)
        return EOF;

    flujo = (t_datos_conector *) iobuf->opaque;

    if (flujo->lgtope == 0) {
lee_mas:
        flujo->lgtope = recv(rt.toma->cliente,
                             flujo->tope + flujo->ptrreg,
                             flujo->max - flujo->ptrreg, 0);

        if (flujo->lgtope <= 0) {
            if (flujo->ptrreg > 0) {
                bzero(flujo->tope + flujo->ptrreg,
                      flujo->max - flujo->ptrreg);

                *out = flujo->tope;
                return flujo->ptrreg;
            }else
                return EOF;
        } 

        /* Limpiar tope sobrante */
        if (((size_t)flujo->lgtope + flujo->ptrreg) < flujo->max)
            bzero(flujo->tope + ((size_t)flujo->lgtope + flujo->ptrreg),
                  flujo->max - ((size_t)flujo->lgtope + flujo->ptrreg));

        flujo->ptareg = flujo->ptrreg;
        flujo->ptrreg = 0;
    } else {
        /* Apunta al siguiente registro del tope */
        flujo->ptrreg += flujo->lgtreg + (int) flujo->tsr;
    }

    /* Apuntar al siguiente registro (variable RT) */
    *rt_start = strstr((const char*) flujo->tope + flujo->ptrreg,
                       (const char*) flujo->sdrt);
    *rt_len = flujo->tsr;

    if (*rt_start == NULL) {
        *rt_len = 0;
        
        /* Copia lo que nos queda por leer al inicio */
        memcpy(flujo->tope, (const void *) (flujo->tope + flujo->ptrreg),
               (flujo->lgtope + flujo->ptareg) - flujo->ptrreg);
        flujo->ptrreg = (flujo->lgtope + flujo->ptareg) - flujo->ptrreg;

        goto lee_mas;
    }

    flujo->lgtreg = long_registro(flujo->tope + flujo->ptrreg, *rt_start);

    *out = flujo->tope + flujo->ptrreg;
    return flujo->lgtreg;
}

/* conector_escribe -- Envía respuesta a solicitud del cliente */

static size_t
conector_escribe(const void *buf, size_t size, size_t count, FILE *fp,
                 void *opaque)
{
    (void) fp;
    (void) opaque;

    if (send(rt.toma->cliente, buf, (size * count), 0) < 0) {
        perror("send");
        lintwarn(ext_id, "conector_escribe: registro demasiado extenso");
        return EOF;
    }

    return (size * count);
}

/* conector_puede_aceptar_fichero -- Decide si aceptamos fichero */

static awk_bool_t
conector_puede_aceptar_fichero(const char *name)
{
    return (   name != NULL
            && strcmp(name, rt.nombre) == 0 /* Toma registrada 'creatoma' */
            && rt.toma->cliente > 0           /* De momento */
            && rt.local);                   /* De momento sólo local */
}

/* conector_tomar_control_de -- Prepara procesador bidireccional */

static awk_bool_t
conector_tomar_control_de(const char *name, awk_input_buf_t *inbuf,
                          awk_output_buf_t *outbuf)
{
    awk_value_t valor_tpm, valor_rs;
    t_datos_conector *flujo;

    (void) name;

    if (   inbuf == NULL || outbuf == NULL
        || !(sym_lookup("TPM", AWK_NUMBER, &valor_tpm)) /* ToPe Máximo */
        || valor_tpm.num_value <= 0
        || !(sym_lookup("RS",  AWK_STRING, &valor_rs))  /* Valor de RS */
       )
        return awk_false;

    /* Memoriza estructura opaca */
    emalloc(flujo, t_datos_conector *,
        sizeof(t_datos_conector), "conector_tomar_control_de");

    flujo->lgtope = 0;
    flujo->lgtreg = 0;
    flujo->ptrreg = 0;

    flujo->tsr = strlen((const char *) valor_rs.str_value.str);
    emalloc(flujo->sdrt, char *,
            flujo->tsr + 1, "conector_tomar_control_de");
    strcpy(flujo->sdrt, (const char *) valor_rs.str_value.str);

    flujo->max = (size_t) valor_tpm.num_value;
    emalloc(flujo->tope, char *,
            flujo->max, "conector_tomar_control_de");
    bzero(flujo->tope, flujo->max);
    libera = 1;

    /* Entrada */
    inbuf->fd = rt.toma->cliente + 1;
    inbuf->opaque = flujo;
    inbuf->get_record = conector_trae_registro;
    inbuf->close_func = cierra_toma_entrada;

    /* Salida */
    outbuf->fp = fdopen(rt.toma->cliente, "w");
    outbuf->opaque = flujo;
    outbuf->gawk_fwrite = conector_escribe;
    outbuf->gawk_fflush = limpia_toma_salida;
    outbuf->gawk_ferror = maneja_error;
    outbuf->gawk_fclose = cierra_toma_salida;
    outbuf->redirected = awk_true;

    return awk_true;
 }

/* Registra nuevo procesador bidireccional llamado 'conector' */

static awk_two_way_processor_t conector_es = {
    "conector",
    conector_puede_aceptar_fichero,
    conector_tomar_control_de,
    NULL
};

/* inicia_conector -- Incicia conector bidireccional */

static awk_bool_t
inicia_conector()
{
    register_two_way_processor(& conector_es);
    return awk_true;
}

#ifdef API_AWK_V2
static awk_ext_func_t lista_de_funciones[] = {
    { "creatoma",   haz_crea_toma,      1, 1, awk_false, NULL },
    { "cierratoma", haz_cierra_toma,    1, 1, awk_false, NULL },
    { "traepctoma", haz_extrae_primera, 2, 1, awk_false, NULL },
};
#else
static awk_ext_func_t lista_de_funciones[] = {
    { "creatoma",   haz_crea_toma,      1 },
    { "cierratoma", haz_cierra_toma,    1 },
    { "traepctoma", haz_extrae_primera, 2 },
};
#endif

/* Cargar funciones */

dl_load_func(lista_de_funciones, conector, "")