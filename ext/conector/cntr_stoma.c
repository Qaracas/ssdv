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

#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netdb.h>

#include "cntr_defcom.h"
#include "cntr_toma.h"
#include "cntr_stoma.h"

/* es_dirip --
 *
 * Modifica criterios evitando a getaddrinfo() resolver nombre
 */

static void
es_dirip(char *ip, struct addrinfo *criterios)
{
    struct sockaddr_in t4;
    struct in6_addr    t6;

    int rc = inet_pton(AF_INET, ip, &t4.sin_addr);
    if (rc == 1) {     /* ¿Dirección IPv4 válida? */
        criterios->ai_family = AF_INET;
        criterios->ai_flags |= AI_NUMERICHOST;
    } else {
        rc = inet_pton(AF_INET6, ip, &t6);
        if (rc == 1) { /* ¿Dirección IPv6 válida? */
            criterios->ai_family = AF_INET6;
            criterios->ai_flags |= AI_NUMERICHOST;
        }
    }
}

/* cntr_nueva_infred */

int
cntr_nueva_infred(char *nodo, char *puerto, t_cntr_toma_es *toma)
{
    if (nodo == NULL || puerto == NULL || toma == NULL)
        return CNTR_ERROR;

    int r;
    struct addrinfo criterios;

    toma->local = cntr_falso;

    /* Criterios para seleccionar las estructuras de tomas IP
     * que 'getaddrinfo()' volcará a la lista 'resultados' */
    memset(&criterios, 0, sizeof(struct addrinfo));
    criterios.ai_family = AF_UNSPEC;     /* Vale IP v4 ó v6 */
    criterios.ai_socktype = SOCK_STREAM; /* Toma de datos sobre TCP */

    if (strcmp(nodo, "0") == 0) {
        criterios.ai_flags = AI_PASSIVE; /* Dirección IP comodín */
        if ((r = getaddrinfo(NULL, puerto, &criterios, &toma->infred)) != 0)
            return CNTR_ERROR;
        toma->local = cntr_cierto;
        return CNTR_HECHO;
    }

     /* Evitar a 'getaddrinfo' resolver nombre */
    es_dirip(nodo, &criterios);
    if ((r = getaddrinfo(nodo, puerto, &criterios, &toma->infred)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
        perror("Error: ");
        return CNTR_ERROR;
    }

    /* Averiguamos si el nodo es local */
    struct ifaddrs *stomas_locales, *i;
    struct addrinfo *j;

    if (getifaddrs(&stomas_locales) == -1)
        return CNTR_ERROR;

    void *resul, *local;
    j = toma->infred;
    while (j) {
        if (j->ai_family == AF_INET)
            resul = &((struct sockaddr_in*)j->ai_addr)->sin_addr;
        else if (j->ai_family == AF_INET6)
            resul = &((struct sockaddr_in6*)j->ai_addr)->sin6_addr;
        else {
            j = j->ai_next;
            continue;
        }
        i = stomas_locales;
        while (i){
            if (i->ifa_addr->sa_family == AF_INET)
                local = &((struct sockaddr_in*)i->ifa_addr)->sin_addr;
            else if (i->ifa_addr->sa_family == AF_INET6)
                local = &((struct sockaddr_in6*)i->ifa_addr)->sin6_addr;
            else {
                i = i->ifa_next;
                continue;
            }
            if (i->ifa_addr->sa_family != j->ai_family) {
                i = i->ifa_next;
                continue;
            }
            if (i->ifa_addr->sa_family == AF_INET) {
                if (   ntohl(((struct in_addr*)local)->s_addr)
                    == ntohl(((struct in_addr*)resul)->s_addr))
                {
                    toma->local = cntr_cierto;
                    goto fin;
                }
            } else {
                if ((memcmp
                        (((struct in6_addr*)local)->s6_addr,
                         ((struct in6_addr*)resul)->s6_addr,
                         sizeof(((struct in6_addr*)resul)->s6_addr))) == 0)
                {
                    toma->local = cntr_cierto;
                    goto fin;
                }
            }
            i = i->ifa_next;
        }
        j = j->ai_next;
    }
fin:
    freeifaddrs(stomas_locales);
    return CNTR_HECHO;
}

/* cntr_borra_infred */

void
cntr_borra_infred(t_cntr_toma_es *toma)
{
    freeaddrinfo(toma->infred);
    toma->infred = NULL;
}