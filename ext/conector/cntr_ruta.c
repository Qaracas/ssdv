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

#include <stdlib.h>
#include <string.h>

#include "cntr_defcom.h"
#include "cntr_cdntxt.h"
#include "cntr_ruta.h"
#include "cntr_toma.h"
#include "cntr_stoma.h"

/*
 * Nombres especiales para los ficheros de red (nombre o id de ruta)
 *
 * Basado en:
 * https://www.gnu.org/software/gawk/manual/html_node/TCP_002fIP-Networking.html
 * 
 * /tipo-red/protocolo/ip-local/puerto-local/nombre-ip-remoto/puerto-remoto
 *
 * Ejemplos: 
 *   - Servidor: /ired/tcp/192.168.1.32/7080/0/0
 *   - Cliente : /ired/tcp/0/0/www.ejemplo.es/8080
 */
int
procesa_nombre_ruta(const char *nombre, t_cntr_ruta **ruta)
{
    if (   nombre == NULL
        || strlen(nombre) < 12
        || cuenta_crtrs(nombre, '/') == CNTR_ERROR
        || cuenta_crtrs(nombre, '/') != 6
        || caracter_ini(nombre) == CNTR_ERROR
        || caracter_fin(nombre) == CNTR_ERROR
        || caracter_ini(nombre) != '/'
        || caracter_fin(nombre) == '/'
       )
        return CNTR_ERROR;

    char *v_nombre;
    cntr_asigmem(v_nombre, char *,
                 strlen((const char *) nombre) + 1,
                 "cntr_nueva_ruta");
    strcpy(v_nombre, (const char *) nombre);

    unsigned int c;
    char *campo[6];
    campo[0] = strtok(v_nombre, "/");
    for (c = 0; (c < cntr_ltd(campo) - 1) && campo[c] != NULL;)
        campo[++c] = strtok(NULL, "/");

    if (   c != (cntr_ltd(campo) - 1)
        || strcmp(campo[0], "ired") != 0
        || (   strcmp(campo[1], "tcp") != 0
            && strcmp(campo[1], "tls") != 0)
        || !es_numero(campo[3])
        || atoi(campo[3]) < 0
        || strcmp(campo[4], "0") != 0
        || strcmp(campo[5], "0") != 0
       )
        return CNTR_ERROR;

    (*ruta)->tipo = strdup(campo[0]);
    (*ruta)->protocolo = strdup(campo[1]);
    (*ruta)->nodo_local = strdup(campo[2]);
    (*ruta)->puerto_local = strdup(campo[3]);
    (*ruta)->nodo_remoto = strdup(campo[4]);
    (*ruta)->puerto_remoto = strdup(campo[5]);

    if (strcmp(campo[1], "tls") == 0)
        (*ruta)->segura = cntr_cierto;
    else
        (*ruta)->segura = cntr_falso;

    free(v_nombre);

    return CNTR_HECHO;
}

/* cntr_nueva_ruta */

int
cntr_nueva_ruta(const char *nombre, t_cntr_ruta **ruta)
{
    cntr_asigmem(*ruta, t_cntr_ruta *,
                 sizeof(t_cntr_ruta),
                 "cntr_nueva_ruta");

    if (procesa_nombre_ruta(nombre, ruta) == CNTR_ERROR) {
        free(ruta);
        ruta = NULL;
        return CNTR_ERROR;
    }

    cntr_asigmem((*ruta)->nombre, char *,
                 strlen((const char *) nombre) + 1,
                 "cntr_nueva_ruta");
    strcpy((*ruta)->nombre, (const char *) nombre);

    (*ruta)->toma = NULL;
    (*ruta)->local = cntr_cierto;

    return CNTR_HECHO;
}

/* cntr_borra_ruta */

void
cntr_borra_ruta(t_cntr_ruta *ruta)
{
    if (ruta->toma != NULL) {
        cntr_borra_infred(ruta->toma);
        cntr_borra_toma(ruta->toma);
    }
    free(ruta->nombre);
    ruta->nombre = NULL;
    free(ruta->tipo);
    ruta->tipo = NULL;
    free(ruta->protocolo);
    ruta->protocolo = NULL;
    free(ruta->nodo_local);
    ruta->nodo_local = NULL;
    free(ruta->puerto_local);
    ruta->puerto_local = NULL;
    free(ruta->nodo_remoto);
    ruta->nodo_remoto = NULL;
    free(ruta->puerto_remoto);
    ruta->puerto_remoto = NULL;
    free(ruta);
    ruta = NULL;
}
