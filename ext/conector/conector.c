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

#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gawkapi.h"

#include <arpa/inet.h>

#include "cntr_defcom.h"
#include "cntr_ruta.h"
#include "cntr_toma.h"
#include "cntr_stoma.h"
#include "cntr_tope.h"
#include "cntr_serie.h"

#define MAX_EVENTOS        10

static const gawk_api_t *api; /* Conveniencia para usar macros */
static awk_ext_id_t ext_id;
static const char *ext_version = "extensión conector: versión 1.0";

static awk_bool_t inicia_conector(void);
static awk_bool_t (*init_func)(void) = inicia_conector;

int plugin_is_GPL_compatible;

/* Variables globales que mantienen su valor */

static t_cntr_ruta *rt;  /* Ruta de conexión */

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
    extern t_cntr_ruta *rt;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs != 1)
        fatal(ext_id, "creatoma: nº de argumentos incorrecto");

    if (! get_argument(0, AWK_STRING, &nombre))
        fatal(ext_id, "creatoma: tipo de argumento incorrecto");

    if (nombre.str_value.str == NULL)
        fatal(ext_id, "creatoma: error leyendo nombre de fichero especial");

    rt = cntr_pon_ruta_en_serie((const char *) nombre.str_value.str);

    if (rt == NULL || !rt->local)
        fatal(ext_id, "creatoma: la ruta ya existe o es incorrecta");

    cntr_nueva_toma(rt);
    if (cntr_pon_a_escuchar_toma(rt) == CNTR_ERROR)
        fatal(ext_id, "creatoma: error creando toma de escucha");

    return make_number(rt->toma->servidor, resultado);
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
    extern t_cntr_ruta *rt;

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

    if (nombre.str_value.str == NULL)
        fatal(ext_id, "cierratoma: error leyendo nombre de fichero especial");

    rt = cntr_busca_ruta_en_serie((const char *) nombre.str_value.str);

    if (rt == NULL) {
        lintwarn(ext_id, "cierratoma: toma de escucha inexistente");
        return make_number(-1, resultado);
    }

    t_elector_es opcn;
    opcn.es_servidor = 1;
    opcn.es_cliente  = 0;
    if (cntr_cierra_toma(rt, opcn) == CNTR_ERROR)
        lintwarn(ext_id, "cierratoma: error cerrando toma");

    //cntr_borra_ruta(rt);
    cntr_borra_ruta_de_serie((const char *) nombre.str_value.str);

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
    extern t_cntr_ruta *rt;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 2 argumentos como máximo */
    if (nargs < 1 || nargs > 2)
        fatal(ext_id, "traepctoma: nº de argumentos incorrecto");

    if (! get_argument(0, AWK_STRING, &valorarg))
        fatal(ext_id, "traepctoma: tipo de argumento incorrecto");

    if (valorarg.str_value.str == NULL)
        fatal(ext_id, "traepctoma: error leyendo nombre de fichero especial");

    rt = cntr_busca_ruta_en_serie((const char *) valorarg.str_value.str);

    if (rt == NULL)
        fatal(ext_id, "traepctoma: toma escucha inexistente");

    struct sockaddr_in cliente;

    if (cntr_trae_primer_cliente_toma(rt,
                                      (struct sockaddr*) &cliente)
                                      == CNTR_ERROR)
        fatal(ext_id, "traepctoma: error creando toma de escucha");

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

    /* Devuelve accept(), es decir, el descriptor de fichero de la 
       toma de conexión recién creada con el cliente. */
    return make_number(rt->toma->cliente, resultado);
}

/**
 * Procesador bidireccional que proporciona 'conector' a GAWK
 */

/* Los datos del puntero oculto */

typedef struct datos_proc_bidireccional {
    t_cntr_tope *tope;  /* Tope de datos entre la E/S                 */
    char        *sdrt;  /* Separador de registro. Variable RS de gawk */
    size_t      tsr;    /* Tamaño cadena separador de registro        */
    size_t      lgtreg; /* Tamaño actual del registro                 */
    int         en_uso;
} t_datos_conector;

/* libera_conector -- Libera datos */

static void
libera_conector(t_datos_conector *dc)
{
    /* Evita liberar memoria dos veces */
    if (dc->en_uso) {
        dc->en_uso--;
        return;
    }
    cntr_borra_tope(dc->tope);
    gawk_free(dc->sdrt);
    gawk_free(dc);
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
    t_datos_conector *dc;

    if (iobuf == NULL || iobuf->opaque == NULL)
        return;

    dc = (t_datos_conector *) iobuf->opaque;
    libera_conector(dc);

    /* haz_cierra_toma() hace esto con la toma de escucha */

    iobuf->fd = INVALID_HANDLE; /* Por ahora es un df ficticio */
}

/* cierra_toma_salida -- Cerrar toma conexión salida */

static int
cierra_toma_salida(FILE *fp, void *opaque)
{
    t_datos_conector *dc;
    extern t_cntr_ruta *rt;

    if (opaque == NULL)
        return EOF;

    dc = (t_datos_conector *) opaque;

    libera_conector(dc);

    t_elector_es opcn;
    opcn.es_servidor = 0;
    opcn.es_cliente  = 1;
    opcn.forzar      = 0;
    if (cntr_cierra_toma(rt, opcn) == CNTR_ERROR)
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
    if (out == NULL || iobuf == NULL || iobuf->opaque == NULL)
        return EOF;

    t_datos_conector *dc;
    int recbt;
    extern t_cntr_ruta *rt;

    (void) errcode;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    dc = (t_datos_conector *) iobuf->opaque;

    if (dc->tope->ldatos == 0) {
lee_mas:
        recbt = cntr_recb_llena_tope(rt, dc->tope);
        switch (recbt) {
            case CNTR_TOPE_RESTO:
                *out = dc->tope->datos;
                return dc->tope->ptrreg;
            case CNTR_TOPE_VACIO:
                return EOF;
            case CNTR_ERROR:
                fatal(ext_id,
                      "conector_trae_registro: error llenando de entrada");
        }
    } else {
        /* Apunta al siguiente registro del tope */
        dc->tope->ptrreg += dc->lgtreg + (int) dc->tsr;
    }

    /* Apuntar al siguiente registro (variable RT) */
    *rt_start = strstr((const char*) dc->tope->datos + dc->tope->ptrreg,
                       (const char*) dc->sdrt);
    *rt_len = dc->tsr;

    if (*rt_start == NULL) {
        *rt_len = 0;
        /* Copia lo que nos queda por leer al inicio del tope */
        memcpy(dc->tope->datos,
               (const void *) (dc->tope->datos + dc->tope->ptrreg),
               (dc->tope->ldatos + dc->tope->ptareg) - dc->tope->ptrreg);
        dc->tope->ptrreg =   (dc->tope->ldatos + dc->tope->ptareg)
                           - dc->tope->ptrreg;
        goto lee_mas;
    }

    dc->lgtreg = long_registro(dc->tope->datos + dc->tope->ptrreg, *rt_start);

    *out = dc->tope->datos + dc->tope->ptrreg;
    return dc->lgtreg;
}

/* conector_escribe -- Envía respuesta a solicitud del cliente */

static size_t
conector_escribe(const void *buf, size_t size, size_t count, FILE *fp,
                 void *opaque)
{
    (void) fp;
    (void) opaque;
    extern t_cntr_ruta *rt;

    if (cntr_envia_a_toma(rt, buf, (size * count)) == CNTR_ERROR) {
        lintwarn(ext_id, "conector_escribe: error enviado registro");
        return EOF;
    }

    return (size * count);
}

/* conector_puede_aceptar_fichero -- Decide si aceptamos fichero */

static awk_bool_t
conector_puede_aceptar_fichero(const char *name)
{
    extern t_cntr_ruta *rt;

    return (   name != NULL
            && strcmp(name, rt->nombre) == 0 /* Toma registrada 'creatoma' */
            && rt->toma->cliente > 0         /* De momento */
            && rt->local);                   /* De momento sólo local */
}

/* conector_tomar_control_de -- Prepara procesador bidireccional */

static awk_bool_t
conector_tomar_control_de(const char *name, awk_input_buf_t *inbuf,
                          awk_output_buf_t *outbuf)
{
    awk_value_t valor_tpm, valor_rs;
    t_datos_conector *dc;
    extern t_cntr_ruta *rt;

    (void) name;

    if (   inbuf == NULL || outbuf == NULL
        || !(sym_lookup("TPM", AWK_NUMBER, &valor_tpm)) /* Tope máximo */
        || valor_tpm.num_value <= 0
        || !(sym_lookup("RS",  AWK_STRING, &valor_rs))  /* Valor de RS */
       )
        return awk_false;

    /* Memoriza estructura opaca */
    emalloc(dc, t_datos_conector *,
        sizeof(t_datos_conector), "conector_tomar_control_de");
    dc->lgtreg = 0;
    dc->en_uso = 1;

    dc->tsr = strlen((const char *) valor_rs.str_value.str);
    emalloc(dc->sdrt, char *,
            dc->tsr + 1, "conector_tomar_control_de");
    strcpy(dc->sdrt, (const char *) valor_rs.str_value.str);

    cntr_nuevo_tope((size_t) valor_tpm.num_value, &dc->tope);

    /* Entrada */
    inbuf->fd = rt->toma->cliente + 1;
    inbuf->opaque = dc;
    inbuf->get_record = conector_trae_registro;
    inbuf->close_func = cierra_toma_entrada;

    /* Salida */
    outbuf->fp = fdopen(rt->toma->cliente, "w");
    outbuf->opaque = dc;
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