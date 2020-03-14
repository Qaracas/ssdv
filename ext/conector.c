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

#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include "gawkapi.h"

static const gawk_api_t *api; /* Conveniencia para usar macros */
static awk_ext_id_t ext_id;
static const char *ext_version = "extensión conector: versión 1.0";

static awk_bool_t inicia_conector(void);
static awk_bool_t (*init_func)(void) = inicia_conector;

int plugin_is_GPL_compatible;

#define PENDIENTES 100

typedef struct descriptores_es {
    int toma_entrada; /* Descriptor fichero servidor en modo escucha */
    int toma_salida;  /* Descriptor fichero cliente (conexión entrante) */
    int libera;       /* Indica memoria liberada */
} t_conector_es;

static t_conector_es dfes;

/* haz_crea_toma -- Crea toma de escucha */

static awk_value_t *
#ifdef API_AWK_V2
haz_crea_toma(int nargs, awk_value_t *resultado, struct awk_ext_func *unused)
#else
haz_crea_toma(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t puerto;
    struct sockaddr_in servidor;

    /* Sólo acepta 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "creatoma: nº de argumentos incorrecto");
        return make_number(-1, resultado);
    }

    if (! get_argument(0, AWK_NUMBER, &puerto)) {   
        lintwarn(ext_id, "creatoma: tipo de argumento incorrecto");
        return make_number(-1, resultado);
    }

    /* Crea toma de entrada */
    dfes.toma_entrada = socket(AF_INET, SOCK_STREAM, 0);

    if (dfes.toma_entrada < 0) {
        lintwarn(ext_id, "creatoma: error creando toma de entrada");
        return make_number(-1, resultado);
    }

    /* Asocia toma de entrada a una dirección IP y un puerto */
    bzero(&servidor, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons((int) puerto.num_value);

    if (bind(dfes.toma_entrada, (struct sockaddr*) &servidor,
            sizeof(servidor)) < 0) {
        lintwarn(ext_id, "creatoma: error enlazando toma de entrada");
        return make_number(-1, resultado);
    }

    /* Poner toma en modo escucha */
    if (listen(dfes.toma_entrada, PENDIENTES) < 0){
        lintwarn(ext_id, "creatoma: error poniendo toma en escucha");
        return make_number(-1, resultado);
    }

    return make_number(dfes.toma_entrada, resultado);
}

/* haz_cierra_toma -- Cierra toma de escucha */
static awk_value_t *
#ifdef API_AWK_V2
haz_cierra_toma(int nargs, awk_value_t *resultado, struct awk_ext_func *unused)
#else
haz_cierra_toma(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t df_cnx_ent;

    /* Sólo acepta 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "cierra: nº de argumentos incorrecto");
        return make_number(-1, resultado);
    }

    if (! get_argument(0, AWK_NUMBER, &df_cnx_ent)) {   
        lintwarn(ext_id, "cierra: tipo de argumento incorrecto");
        return make_number(-1, resultado);
    }

    if (((int) df_cnx_ent.num_value) != dfes.toma_entrada) {
        lintwarn(ext_id, "cierra: toma escucha incorrecta");
        return make_number(-1, resultado);
    }

    if (close(dfes.toma_entrada) < 0) {
        perror("cierra");
        lintwarn(ext_id, "cierra: error cerrando toma de conexión");
        return make_number(-1, resultado);
    }

    return make_number(0, resultado);
}

/* haz_extrae_primera -- Extrae primera conexión de la cola de conexiones */

static awk_value_t *
#ifdef API_AWK_V2
haz_extrae_primera(int nargs, awk_value_t *resultado,
                   struct awk_ext_func *unused)
#else
haz_extrae_primera(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t toma_ent;
    struct sockaddr_in cliente;
    socklen_t lnt = (socklen_t) sizeof(cliente);

    /* Sólo acepta 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "extraep: nº de argumentos incorrecto");
        return make_number(-1, resultado);
    }

    if (! get_argument(0, AWK_NUMBER, &toma_ent)) {
        lintwarn(ext_id, "extraep: tipo de argumento incorrecto");
        return make_number(-1, resultado);
    }

    if (((int) toma_ent.num_value) != dfes.toma_entrada) {
        lintwarn(ext_id, "cierra: toma escucha incorrecta");
        return make_number(-1, resultado);
    }

    /* Extrae la primera conexión de la cola de conexiones */
    dfes.toma_salida = accept(dfes.toma_entrada, 
                        (struct sockaddr*) &cliente, &lnt);

    if (dfes.toma_salida < 0) {
        lintwarn(ext_id, "creatoma: error enlazando toma de entrada");
    }

    /* Devuelve accept(), es decir, el descriptor de fichero de la 
       toma de conexión recién creada con el cliente. Valor de la 
       variable global DFC (Descriptor Fichero Cliente) */
    return make_number(dfes.toma_salida, resultado);
}

/* Los datos del puntero oculto */

typedef struct datos_proc_bidireccional {
    char *tope;    /* Tope de datos */
    char *sdrt;    /* Separador de registro. Variable RS de gawk */
    size_t tsr;    /* Tamaño cadena separador de registro */
    size_t max;    /* Tamaño máximo asignado al tope */
    size_t lgtope; /* Tamaño actual del tope */
    size_t lgtreg; /* Tamaño actual del registro */
    int ptrreg;    /* Puntero inicio registro en tope (actual) */
    int ptareg;    /* Puntero inicio registro en tope (anterior) */
} t_datos_conector;

/* libera_conector --- Libera datos */

static void
libera_conector(t_datos_conector *flujo)
{
    /* Evita liberar memoria dos veces */
    if (dfes.libera > 0)
        dfes.libera--;
    else
        return;

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

    (void) iobuf->fd; /* No se cierra: enlazado a toma de escucha */
    close(dfes.toma_salida);

    iobuf->fd = INVALID_HANDLE;
}

/* cierra_toma_salida --- Cerrar toma conexión salida */

static int
cierra_toma_salida(FILE *fp, void *opaque)
{
    t_datos_conector *flujo;

    if (opaque == NULL)
        return EOF;

    flujo = (t_datos_conector *) opaque;
    libera_conector(flujo);

    /* Flujo y descriptor de salida (conexión cliente) */
    fclose(fp);
    close(dfes.toma_salida);
    
    return 0;
}

/* apaga_toma_salida --- Apagar toma de salida */

static int
apaga_toma_salida(FILE *fp, void *opaque)
{
    (void) fp;
    (void) opaque;

    return 0;
}

/* maneja_error --- De momento no hacer nada */

static int
maneja_error(FILE *fp, void *opaque)
{
    (void) fp;
    (void) opaque;

    return 0;
}

/* conector_trae_registro -- lee un registro cada vez */

static int
#ifdef API_AWK_V2
conector_trae_registro(char **out, awk_input_buf_t *iobuf, int *errcode,
                       char **rt_start, size_t *rt_len,
                       const awk_fieldwidth_info_t **unused)
#else
conector_trae_registro(char **out, awk_input_buf_t *iobuf, int *errcode,
                       char **rt_start, size_t *rt_len)
#endif
{
    t_datos_conector *flujo;

    (void) errcode; /* silenciar alertas */
    if (out == NULL || iobuf == NULL || iobuf->opaque == NULL)
        return EOF;

    flujo = (t_datos_conector *) iobuf->opaque;

    if (flujo->lgtope == 0) {
lee_mas:
        flujo->lgtope = recv(dfes.toma_salida, 
                             flujo->tope + flujo->ptrreg,
                             flujo->max - flujo->ptrreg, 0);

        if (flujo->lgtope == 0)
            return EOF;

        if (flujo->lgtope < 0) {
            update_ERRNO_int(ENOENT);
            update_ERRNO_string("conector: error de E/S");
            lintwarn(ext_id,
                     "trae_regsitro: error recibiendo datos de la toma");
            return EOF;
        }

        flujo->ptareg = flujo->ptrreg;
        flujo->ptrreg = 0;
    } else {
        /* Apunta al siguiente registro del tope */
        flujo->ptrreg += flujo->lgtreg + (int) flujo->tsr;
    }

    /* Apuntar a los datos que se utilizarán para RT */
    *rt_start = strstr((const char*) flujo->tope + flujo->ptrreg,
                       (const char*) flujo->sdrt);
    *rt_len = flujo->tsr;

    if (*rt_start == NULL) {
        if (flujo->ptrreg == 0) {
            update_ERRNO_int(EOVERFLOW);
            update_ERRNO_string("conector: tope desbordado");
            lintwarn(ext_id, "trae_registro: registro demasiado extenso");
            return EOF;
        }

        /* Copia lo que nos queda por leer a 'rsto' */
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
    t_datos_conector *flujo;

    if (opaque == NULL)
        return EOF;

    //(void) fp;
    flujo = (t_datos_conector *) opaque;

    if (send(dfes.toma_salida, buf, (size * count), 0) < 0) {
        lintwarn(ext_id, "conector_escribe: registro demasiado extenso");
        return EOF;
    }

    return (size * count);
}

/* conector_puede_aceptar_fichero -- retorna "true" si aceptamos fichero */

static awk_bool_t
conector_puede_aceptar_fichero(const char *name)
{
    return (name != NULL && strcmp(name, "/ired") == 0);
}

/* conector_tomar_control_de -- prepara procesador bidireccional */

static awk_bool_t
conector_tomar_control_de(const char *name, awk_input_buf_t *inbuf,
                          awk_output_buf_t *outbuf)
{
    awk_value_t valor_tpm, valor_rs;
    t_datos_conector *flujo;

    (void) name; /* silenciar avisos */
    if (   inbuf == NULL || outbuf == NULL
        || !(sym_lookup("TPM", AWK_NUMBER, &valor_tpm)) /* ToPe Máximo */
        || valor_tpm.num_value <= 0
        || !(sym_lookup("RS",  AWK_STRING, &valor_rs))  /* Valor de RS */
       )
        return awk_false;

    if (dfes.toma_salida < 0)
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

    dfes.libera = 1;
    flujo->max = (size_t) valor_tpm.num_value;
    emalloc(flujo->tope, char *,
            flujo->max, "conector_tomar_control_de");
    
    /* Entrada */
    inbuf->fd = dfes.toma_entrada;
    inbuf->opaque = flujo;
    inbuf->get_record = conector_trae_registro;
    inbuf->close_func = cierra_toma_entrada;

    /* Salida */
    outbuf->fp = fdopen(dfes.toma_salida, "w");
    outbuf->opaque = flujo;
    outbuf->gawk_fwrite = conector_escribe;
    outbuf->gawk_fflush = apaga_toma_salida;
    outbuf->gawk_ferror = maneja_error;
    outbuf->gawk_fclose = cierra_toma_salida;
    outbuf->redirected = awk_true;

    return awk_true;
 }

static awk_two_way_processor_t conector_es = {
    "conector",
    conector_puede_aceptar_fichero,
    conector_tomar_control_de,
    NULL
};

/* inicia_conector --- Incicia conector bidireccional */

static awk_bool_t
inicia_conector()
{
    register_two_way_processor(& conector_es);
    return awk_true;
}

#ifdef API_AWK_V2
static awk_ext_func_t lista_de_funciones[] = {
    { "creatoma",   haz_crea_toma,      0, 0, awk_false, NULL },
    { "cierratoma", haz_cierra_toma,    0, 0, awk_false, NULL },
    { "extraep",    haz_extrae_primera, 0, 0, awk_false, NULL },
};
#else
static awk_ext_func_t lista_de_funciones[] = {
    { "creatoma",   haz_crea_toma,      0 },
    { "cierratoma", haz_cierra_toma,    0 },
    { "extraep",    haz_extrae_primera, 0 },
};
#endif
/* Define función de carga */

dl_load_func(lista_de_funciones, conector, "")
