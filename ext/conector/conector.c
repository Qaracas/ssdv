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

static const gawk_api_t *api; /* Conveniencia para usar macros */
static awk_ext_id_t ext_id;
static const char *ext_version = "extensión conector: versión 1.0";

static awk_bool_t inicia_conector(void);
static awk_bool_t (*init_func)(void) = inicia_conector;

int plugin_is_GPL_compatible;

/* Variables global (puntero oculto) */

static t_cntr_ruta *rt;  /* Ruta de conexión en uso */

/* pon_num_en_coleccion --
 *
 * Añadir elemento numérico a la colección
 */

static void
pon_num_en_coleccion(awk_array_t coleccion, const char *sub, double num)
{
    awk_value_t index, value;

    set_array_element(coleccion,
        make_const_string(sub, strlen(sub), &index),
        make_number(num, &value));
}

/* pon_txt_en_coleccion --
 * 
 * Añadir elemento textual a la colección
 */

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

/* haz_crea_toma --
 *
 * Crea una toma de datos y escucha en ella
 */

static awk_value_t *
#ifdef API_AWK_V2
haz_crea_toma(int nargs, awk_value_t *resultado,
              struct awk_ext_func *desusado)
#else
haz_crea_toma(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t nombre, valor_tpm, valor_rs;
    extern t_cntr_ruta *rt;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs != 1)
        fatal(ext_id, "creatoma: nº de argumentos incorrecto");

    if (! get_argument(0, AWK_STRING, &nombre))
        fatal(ext_id, "creatoma: tipo de argumento incorrecto");

    const char *nombre_ruta = (const char *) nombre.str_value.str;

    if (nombre_ruta == NULL)
        fatal(ext_id, "creatoma: error leyendo nombre de fichero especial");

    if (   (   rt != NULL
            && strcmp(nombre_ruta, (const char *)rt->nombre) == 0)
        || (rt = cntr_busca_ruta_en_serie(nombre_ruta)) != NULL) {
        lintwarn(ext_id, "creatoma: la ruta ya existe");
        return make_number(rt->toma->servidor, resultado);
    }

    if (cntr_nueva_ruta(nombre_ruta, &rt) == CNTR_ERROR) {
        cntr_borra_ruta(rt);
        fatal(ext_id, "creatoma: error creando ruta");
    }

    /* Fuera de ssdv esto no sería un error */
    if (!rt->local) {
        cntr_borra_ruta(rt);
        fatal(ext_id, "creatoma: la ruta no es local");
    }

    if (cntr_nueva_toma(rt) == NULL)
        fatal(ext_id, "creatoma: error creando toma de datos");

    if (!(sym_lookup("TPM", AWK_NUMBER, &valor_tpm)))
        fatal(ext_id, "creatoma: error leyendo variable TPM");

    if (valor_tpm.num_value < 0)
        fatal(ext_id, "creatoma: valor de TPM incorrecto");

    if (!(sym_lookup("RS",  AWK_STRING, &valor_rs)))
        fatal(ext_id, "creatoma: error leyendo variable RS");

    size_t tpm = (size_t) valor_tpm.num_value;
    char * rs = (char *)valor_rs.str_value.str;

    cntr_nueva_estructura_datos_toma(rt->toma, rs, tpm);

    if (cntr_nueva_infred(rt->nodo_local, rt->puerto_local,
                          rt->toma) == CNTR_ERROR)
        fatal(ext_id, "creatoma: error extrayendo información de red");

    if (cntr_pon_a_escuchar_toma(rt->toma) == CNTR_ERROR)
        fatal(ext_id, "creatoma: error poniendo en modo escucha");

    if (cntr_pon_ruta_en_serie(rt) == NULL) {
        cntr_borra_ruta(rt);
        fatal(ext_id, "creatoma: error registrando ruta");
    }

    return make_number(rt->toma->servidor, resultado);
}

/* haz_mata_toma --
 *
 * Borra toma de datos de la memoria y de la serie
 */

static awk_value_t *
#ifdef API_AWK_V2
haz_mata_toma(int nargs, awk_value_t *resultado,
              struct awk_ext_func *desusado)
#else
haz_mata_toma(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t nombre;
    extern t_cntr_ruta *rt;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs != 1)
        fatal(ext_id, "matatoma: nº de argumentos incorrecto");

    if (! get_argument(0, AWK_STRING, &nombre))
        fatal(ext_id, "matatoma: tipo de argumento incorrecto");

    const char *nombre_ruta = (const char *) nombre.str_value.str;

    if (nombre_ruta == NULL)
        fatal(ext_id, "matatoma: error leyendo nombre de fichero especial");

    if (   (   rt != NULL
            && strcmp(nombre_ruta, (const char *)rt->nombre) == 0)
        || (rt = cntr_busca_ruta_en_serie(nombre_ruta)) != NULL) {
        cntr_borra_ruta_de_serie(nombre_ruta);
        cntr_borra_ruta(rt);
    } else {
        lintwarn(ext_id, "matatoma: la ruta no existe");
        return make_number(-1, resultado);
    }

    return make_number(0, resultado);
}

/* haz_acaba_toma_srv --
 * 
 * Cierra toma de datos local
 */

static awk_value_t *
#ifdef API_AWK_V2
haz_acaba_toma_srv(int nargs, awk_value_t *resultado,
                    struct awk_ext_func *desusado)
#else
haz_acaba_toma_srv(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t nombre;
    extern t_cntr_ruta *rt;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "acabasrv: nº de argumentos incorrecto");
        return make_number(-1, resultado);
    }

    if (! get_argument(0, AWK_STRING, &nombre)) {
        lintwarn(ext_id, "acabasrv: tipo de argumento incorrecto");
        return make_number(-1, resultado);
    }

    const char *nombre_ruta = (const char *) nombre.str_value.str;

    if (nombre_ruta == NULL)
        fatal(ext_id, "acabasrv: error leyendo nombre de fichero especial");

    if (   rt != NULL
        && strcmp(nombre_ruta, (const char *)rt->nombre) != 0)
        rt = cntr_busca_ruta_en_serie(nombre_ruta);

    if (rt == NULL) {
        lintwarn(ext_id, "acabasrv: toma de datos inexistente");
        return make_number(-1, resultado);
    }

    t_elector_es opcn;
    opcn.es_servidor = 1;
    opcn.es_cliente  = 0;
    opcn.forzar      = 0;
    if (cntr_cierra_toma(rt->toma, opcn) == CNTR_ERROR)
        lintwarn(ext_id, "acabasrv: error cerrando toma");

    return make_number(0, resultado);
}

/* haz_acaba_toma_cli --
 * 
 * Cierra toma de datos local
 */

static int
cierra_toma_salida(FILE *fp, void *opaque);

static awk_value_t *
#ifdef API_AWK_V2
haz_acaba_toma_cli(int nargs, awk_value_t *resultado,
                    struct awk_ext_func *desusado)
#else
haz_acaba_toma_cli(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t nombre;
    extern t_cntr_ruta *rt;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "acabasrv: nº de argumentos incorrecto");
        return make_number(-1, resultado);
    }

    if (! get_argument(0, AWK_STRING, &nombre)) {
        lintwarn(ext_id, "acabasrv: tipo de argumento incorrecto");
        return make_number(-1, resultado);
    }

    const char *nombre_ruta = (const char *) nombre.str_value.str;

    if (nombre_ruta == NULL)
        fatal(ext_id, "acabasrv: error leyendo nombre de fichero especial");

    if (   rt != NULL
        && strcmp(nombre_ruta, (const char *)rt->nombre) != 0)
        rt = cntr_busca_ruta_en_serie(nombre_ruta);

    if (rt == NULL) {
        lintwarn(ext_id, "acabasrv: toma de datos inexistente");
        return make_number(-1, resultado);
    }

    cierra_toma_salida(NULL, NULL);

    return make_number(0, resultado);
}

/* haz_trae_primer_cli --
 *
 * Extrae primera conexión de la cola de conexiones
 */

static awk_value_t *
#ifdef API_AWK_V2
haz_trae_primer_cli(int nargs, awk_value_t *resultado,
                    struct awk_ext_func *desusado)
#else
haz_trae_primer_cli(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t valorarg;
    extern t_cntr_ruta *rt;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 2 argumentos como máximo */
    if (nargs < 1 || nargs > 2)
        fatal(ext_id, "traeprcli: nº de argumentos incorrecto");

    if (! get_argument(0, AWK_STRING, &valorarg))
        fatal(ext_id, "traeprcli: tipo de argumento incorrecto");

    const char *nombre_ruta = (const char *) valorarg.str_value.str;

    if (nombre_ruta == NULL)
        fatal(ext_id, "traeprcli: error leyendo nombre de fichero especial");

    rt = cntr_busca_ruta_en_serie(nombre_ruta);

    if (rt == NULL)
        fatal(ext_id, "traeprcli: toma de datos inexistente");

    struct sockaddr_in cliente;

    if (cntr_trae_primer_cliente_toma(rt->toma,
                                      (struct sockaddr*) &cliente)
                                      == CNTR_ERROR)
        fatal(ext_id, "traeprcli: error creando toma de datos");

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
                         "traeprcli: segundo argumento incorrecto");
            }
        }
    }

    /* Devuelve accept(), es decir, el descriptor de fichero de la 
       toma de datos recién creada para comunicar con el cliente. */
    return make_number(rt->toma->cliente, resultado);
}

/**
 * Procesador bidireccional que proporciona 'conector' a GAWK
 */

/* Los datos del puntero oculto */

// Ver anterior definición de ruta 'rt'
typedef char * (*recibe_toma)(t_cntr_toma_es *toma, char **sdrt, size_t *tsr,
                              int *resul);

/* cierra_toma_entrada --
 *
 * Cerrar toma de entrada
 */

static void
cierra_toma_entrada(awk_input_buf_t *iobuf)
{
    extern t_cntr_ruta *rt;

    if (iobuf == NULL)
        return;

    /* haz_acaba_toma_srv() hace esto con la toma de escucha */

    iobuf->fd = INVALID_HANDLE; /* Por ahora es un df ficticio */
}

/* cierra_toma_salida --
 *
 * Cerrar toma de salida
 */

static int
cierra_toma_salida(FILE *fp, void *opaque)
{
    extern t_cntr_ruta *rt;
    (void) opaque;

    t_elector_es opcn;
    opcn.es_servidor = 0;
    opcn.es_cliente  = 1;
    opcn.forzar      = 0;
    if (cntr_cierra_toma(rt->toma, opcn) == CNTR_ERROR)
        lintwarn(ext_id, "conector: error cerrando toma con cliente");

    /* Se usa cntr_cierra_toma(), pero lo cerramos */
    fclose(fp);

    return 0;
}

/* limpia_toma_salida --
 *
 * Apagar toma de salida
 */

static int
limpia_toma_salida(FILE *fp, void *opaque)
{
    (void) fp;
    (void) opaque;

    return 0;
}

/* maneja_error --
 *
 * De momento no hacer nada
 */

static int
maneja_error(FILE *fp, void *opaque)
{
    (void) fp;
    (void) opaque;

    return 0;
}

/* conector_recibe_datos --
 *
 * Lee un registro cada vez
 */

static int
#ifdef API_AWK_V2
conector_recibe_datos(char **out, awk_input_buf_t *iobuf, int *errcode,
                      char **rt_start, size_t *rt_len,
                      const awk_fieldwidth_info_t **desusado)
#else
conector_recibe_datos(char **out, awk_input_buf_t *iobuf, int *errcode,
                      char **rt_start, size_t *rt_len)
#endif
{
    if (out == NULL || iobuf == NULL)
        return EOF;

    extern t_cntr_ruta *rt;
    int resul;

    (void) errcode;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    recibe_toma recibe = &cntr_recibe_linea_toma;

    *out = (*recibe)(rt->toma, rt_start, rt_len, &resul);

    if (resul == CNTR_ERROR)
        fatal(ext_id, "conector_recibe_datos: error leyendo de la toma");

    return rt->toma->pila->lgtreg;
}

/* conector_envia_datos --
 *
 * Envía respuesta a solicitud del cliente
 */

static size_t
conector_envia_datos(const void *buf, size_t size, size_t count, FILE *fp,
                     void *opaque)
{
    (void) fp;
    (void) opaque;
    extern t_cntr_ruta *rt;

    if (cntr_envia_toma(rt->toma, buf, (size * count)) == CNTR_ERROR) {
        lintwarn(ext_id, "conector_envia_datos: error enviado registro");
        return EOF;
    }

    return (size * count);
}

/* conector_puede_aceptar_fichero --
 *
 * Decide si aceptamos fichero
 */

static awk_bool_t
conector_puede_aceptar_fichero(const char *name)
{
    extern t_cntr_ruta *rt;

    return (   name != NULL
            && (   strcmp(name, rt->nombre) == 0 /* 1) ¿Es toma en uso?
                                                    2) ¿Está registrada?
                                                       ('creatoma')        */
                || (rt = cntr_busca_ruta_en_serie(name)) != NULL));
}

/* conector_tomar_control_de --
 *
 * Prepara procesador bidireccional
 */

static awk_bool_t
conector_tomar_control_de(const char *name, awk_input_buf_t *inbuf,
                          awk_output_buf_t *outbuf)
{
    extern t_cntr_ruta *rt;

    (void) name;

    /* Entrada */
    inbuf->fd = rt->toma->cliente + 1;
    inbuf->opaque = NULL;
    inbuf->get_record = conector_recibe_datos;
    inbuf->close_func = cierra_toma_entrada;

    /* Salida */
    outbuf->fp = fdopen(rt->toma->cliente, "w");
    outbuf->opaque = NULL;
    outbuf->gawk_fwrite = conector_envia_datos;
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

/* inicia_conector --
 *
 * Incicia conector bidireccional
 */

static awk_bool_t
inicia_conector()
{
    register_two_way_processor(&conector_es);
    return awk_true;
}

/* Define nuevos comantos para GAWK */

#ifdef API_AWK_V2
static awk_ext_func_t lista_de_funciones[] = {
    { "creatoma", haz_crea_toma,       1, 1, awk_false, NULL },
    { "matatoma", haz_mata_toma,       1, 1, awk_false, NULL },
    { "acabasrv", haz_acaba_toma_srv,  1, 1, awk_false, NULL },
    { "acabacli", haz_acaba_toma_cli,  1, 1, awk_false, NULL },
    { "traepcli", haz_trae_primer_cli, 2, 1, awk_false, NULL },
};
#else
static awk_ext_func_t lista_de_funciones[] = {
    { "creatoma", haz_crea_toma,       1 },
    { "matatoma", haz_mata_toma,       1 },
    { "acabasrv", haz_acaba_toma_srv,  1 },
    { "acabacli", haz_acaba_toma_cli,  1 },
    { "traepcli", haz_trae_primer_cli, 2 },
};
#endif

/* Cargar funciones */

dl_load_func(lista_de_funciones, conector, "")