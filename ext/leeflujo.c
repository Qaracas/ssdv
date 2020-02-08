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

#include <stdio.h>
#include <errno.h>

#include <sys/stat.h>

#include "gawkapi.h"

static const gawk_api_t *api;   /* Conveniencia para usar macros */
static awk_ext_id_t ext_id;
static const char *ext_version = "extensión leeflujo: versión 1.0";

static awk_bool_t inicia_leeflujo(void);
static awk_bool_t (*init_func)(void) = inicia_leeflujo;

int plugin_is_GPL_compatible;

/* Tipo de dato para el puntero opaco */

typedef struct fichero_abierto {
    FILE *flujo;
    char *tope;
    size_t max;     /* Por defecto. Si no, vale TPM */
} t_fichero_abierto;

/* trae_registro --- Lee cada vez hasta MAX octetos */

#ifdef AWK
static int
trae_registro(char **out, awk_input_buf_t *iobuf, int *errcode,
        char **rt_start, size_t *rt_len,
        const awk_fieldwidth_info_t **unused)
#else
static int
trae_registro(char **out, awk_input_buf_t *iobuf, int *errcode,
        char **rt_start, size_t *rt_len)
#endif
{
    int ltd = 0;
    t_fichero_abierto *fichero;

    if (out == NULL || iobuf == NULL || iobuf->opaque == NULL)
        return EOF;

    fichero = (t_fichero_abierto *) iobuf->opaque;

    if (feof(fichero->flujo))
        return EOF;

    ltd = fread(fichero->tope, (size_t)1, fichero->max, fichero->flujo);

    *(fichero->tope + ltd + 1) = '\0';
    *out = fichero->tope;

    *rt_start = NULL;
    *rt_len = 0;    /* Pon RT a "" */
    return ltd;
}

/* cerrar_fichero --- Cerrar al acabar */

static void
cerrar_fichero(awk_input_buf_t *iobuf)
{
    t_fichero_abierto *fichero;

    if (iobuf == NULL || iobuf->opaque == NULL)
        return;

    fichero = (t_fichero_abierto *) iobuf->opaque;

    fclose(fichero->flujo);
    gawk_free(fichero->tope);
    gawk_free(fichero);

    iobuf->fd = -1;
}

/*
 * lee_puede_aceptar_fichero --- Retorna "true" si procesamos archivo 
 */
static awk_bool_t
lee_puede_aceptar_fichero(const awk_input_buf_t *iobuf)
{
    awk_value_t valor_tpm;

    if (iobuf == NULL || 
        !(sym_lookup("TPM", AWK_NUMBER, &valor_tpm)) || 
        valor_tpm.num_value <= 0)
        return awk_false;

    return (iobuf->fd != INVALID_HANDLE &&
            (S_ISREG(iobuf->sbuf.st_mode) ||
             S_ISBLK(iobuf->sbuf.st_mode) ||
             S_ISLNK(iobuf->sbuf.st_mode)));
}

/*
 * lee_tomar_control_de --- Activa análisis de entrada.
 */
static awk_bool_t
lee_tomar_control_de(awk_input_buf_t *iobuf)
{
    FILE *flujo;
    awk_value_t valor_tpm;
    t_fichero_abierto *fichero;

    if (!(sym_lookup("TPM", AWK_NUMBER, &valor_tpm)) || 
        valor_tpm.num_value <= 0)
        return awk_false;

    flujo = fdopen(iobuf->fd, "r");

    if (flujo == NULL) {
        update_ERRNO_int(ENOENT);
        update_ERRNO_string("No existe fichero");
        warning(ext_id, "lee_tomar_control_de: error abriendo fichero");
        return awk_false;
    }
    emalloc(fichero, t_fichero_abierto *,
        sizeof(t_fichero_abierto), "lee_tomar_control_de");
    fichero->flujo = flujo;
    fichero->max = (size_t)valor_tpm.num_value;
    emalloc(fichero->tope, char *, (fichero->max) + 1, "lee_tomar_control_de");

    iobuf->opaque = fichero;
    iobuf->get_record = trae_registro;
    iobuf->close_func = cerrar_fichero;

    return awk_true;
}

static awk_input_parser_t procesador_leeflujo = {
    "leeflujo",
    lee_puede_aceptar_fichero,
    lee_tomar_control_de,
    NULL
};

/* inicia_leeflujo --- Iniciar cosas */

static awk_bool_t
inicia_leeflujo()
{
    register_input_parser(& procesador_leeflujo);
    return awk_true;
}

/* No se proporcionan nuevas funciones */

#ifdef AWK
static awk_ext_func_t lista_de_funciones[] = {
    { NULL, NULL, 0, 0, awk_false, NULL }
};
#else
static awk_ext_func_t lista_de_funciones[] = {
    { NULL, NULL, 0 }
};
#endif


/* Define función de carga */

dl_load_func(lista_de_funciones, leeflujo, "")
