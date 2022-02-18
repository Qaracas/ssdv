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

#define _GNU_SOURCE

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "cntr_defcom.h"
#include "cntr_ruta.h"
#include "cntr_toma.h"
#include "cntr_stoma.h"
#include "cntr_tope.h"
#include "cntr_capa_tls.h"

#if GNU_LINUX
#include <sys/epoll.h>
#else
#include <sys/select.h>
#endif

/* cntr_nueva_toma */

t_cntr_toma_es *
cntr_nueva_toma(t_cntr_ruta *ruta)
{
    if (ruta == NULL)
        return NULL;

    cntr_asigmem(ruta->toma, t_cntr_toma_es *,
                 sizeof(t_cntr_toma_es), "cntr_nueva_toma");
#if GNU_LINUX
    cntr_asigmem(ruta->toma->sonda, t_cntr_sonda *,
                 sizeof(t_cntr_sonda), "cntr_nueva_toma");
#endif
    ruta->toma->servidor = CNTR_DF_NULO;
    ruta->toma->cliente = CNTR_DF_NULO;

    if (ruta->segura) {
        cntr_asigmem(ruta->toma->gtls, t_capa_gnutls *,
                     sizeof(t_capa_gnutls), "cntr_nueva_toma");
        ruta->toma->gtls->usándose = 1;
        ruta->toma->envia = &cntr_envia_datos_capa_tls;
        ruta->toma->recibe = &cntr_recibe_datos_capa_tls;
    } else {
        ruta->toma->gtls = NULL;
        ruta->toma->envia = &cntr_envia_datos;
        ruta->toma->recibe = &cntr_recibe_datos;
    }

    return ruta->toma;
}

/* cntr_borra_toma */

void
cntr_borra_toma(t_cntr_toma_es *toma)
{
    free(toma->pila);
    if (toma->gtls != NULL) {
        cntr_liberta_capa_toma_tls(toma->gtls);
        free(toma->gtls);
    }
#if GNU_LINUX
    free(toma->sonda);
#endif
    free(toma);
    toma = NULL;
}

/* cntr_nueva_estructura_datos_toma */

t_cntr_dts_toma *
cntr_nueva_estructura_datos_toma(t_cntr_toma_es *toma, char *sr, size_t tpm)
{
    cntr_asigmem(toma->pila, t_cntr_dts_toma *,
                 sizeof(t_cntr_dts_toma), "cntr_nueva_estructura_datos_toma");
    toma->pila->lgtreg = 0;
    toma->pila->en_uso = 1;

    toma->pila->tsr = strlen((const char *) sr);
    cntr_asigmem(toma->pila->sdrt, char *,
                 toma->pila->tsr + 1, "cntr_nueva_estructura_datos_toma");
    strcpy(toma->pila->sdrt, (const char *) sr);

    cntr_nuevo_tope(tpm, &toma->pila->tope);

    return toma->pila;
}

/* cntr_envia_datos */

ssize_t
cntr_envia_datos(t_capa_gnutls *capatls, int df_cliente,
                 const void *tope, size_t bulto)
{
    (void) capatls;
    return send(df_cliente, tope, bulto, 0);
}

/* cntr_envia_a_toma */

int
cntr_envia_toma(t_cntr_toma_es *toma, const void *datos, size_t bulto)
{
    if ((*toma->envia)(toma->gtls, toma->cliente, datos, bulto) < 0) {
        perror("send");
        return CNTR_ERROR;
    }
    return CNTR_HECHO;
}

/* cntr_recibe_toma */

char *
cntr_recibe_linea_toma(t_cntr_toma_es *toma, char **sdrt, size_t *tsr,
                       t_cntr_resultado **resul)
{
    int recbt;
    struct sockaddr_in cliente;
    t_cntr_tope *tope = toma->pila->tope;

    if (tope->ldatos == 0) {
reintenta_recibir_linea:
        errno = 0;
        recbt = cntr_rcbl_llena_tope(toma);

        switch (recbt) {
            case CNTR_TOPE_RESTO:
                return tope->datos;
            case CNTR_TOPE_VACIO:
                return NULL;
            case CNTR_REINTENTAR:
                if (cntr_trae_primer_cliente_toma(toma,
                                                  (struct sockaddr*) &cliente)
                                                  == CNTR_ERROR)
                *resul = cntr_nuevo_resultado(errno, CNTR_ERROR,
                                              "error leyendo toma");
                goto reintenta_recibir_linea;
            case CNTR_ERROR:
                *resul = cntr_nuevo_resultado(errno, CNTR_ERROR,
                                              "error leyendo toma");
                return NULL;
        }

    } else {
        /* Apunta al siguiente registro del tope */
        tope->ptrreg += toma->pila->lgtreg + toma->pila->tsr;
    }

    /* Apuntar al siguiente separador de registro */
    *sdrt = strstr((const char*) tope->datos + tope->ptrreg,
                       (const char*) toma->pila->sdrt);
    *tsr = toma->pila->tsr;

    if (*sdrt == NULL) {
        if (tope->ptrreg == 0) {
            *resul = cntr_nuevo_resultado(0, CNTR_ERROR,
                                          "desbordamiento de pila");
            return NULL;
        }
        *tsr = 0;
        /* Copia lo que nos queda por leer al inicio del tope */
        memcpy(tope->datos,
               (const void *) (tope->datos + tope->ptrreg),
               (tope->ldatos + tope->ptareg) - tope->ptrreg);
        tope->ptrreg = (tope->ldatos + tope->ptareg) - tope->ptrreg;
        tope->ldatos = 0;
        return cntr_recibe_linea_toma(toma, sdrt, tsr, resul);
    }

    /* Tamaño del registro */
    toma->pila->lgtreg = *sdrt - (tope->datos + tope->ptrreg);

    return tope->datos + tope->ptrreg;
}

/* cntr_recibe_flujo_toma */

char *
cntr_recibe_flujo_toma(t_cntr_toma_es *toma, char **sdrt, size_t *tsr,
                       t_cntr_resultado **resul)
{
    int recbt;
    struct sockaddr_in cliente;
    t_cntr_tope *tope = toma->pila->tope;

reintenta_recibir_flujo:
    errno = 0;
    recbt = cntr_rcbf_llena_tope(toma);

    switch (recbt) {
        case CNTR_TOPE_VACIO:
            return NULL;
        case CNTR_REINTENTAR:
            if (cntr_trae_primer_cliente_toma(toma,
                                              (struct sockaddr*) &cliente)
                                              == CNTR_ERROR)
            *resul = cntr_nuevo_resultado(errno, CNTR_ERROR,
                                          "error leyendo toma");
            goto reintenta_recibir_flujo;
        case CNTR_ERROR:
            *resul = cntr_nuevo_resultado(errno, CNTR_ERROR,
                                          "error leyendo toma");
            return NULL;
    }

    /* Tamaño del registro */
    toma->pila->lgtreg = tope->ldatos;

    /* Variable RT no tiene sentido leyendo flujos */
    *sdrt = NULL;
    *tsr = 0;

    return tope->datos;
}

/* cntr_pon_a_escuchar_toma */

int
cntr_pon_a_escuchar_toma(t_cntr_toma_es *toma)
{
    if (   toma == NULL || toma->infred == NULL
        || toma->servidor != CNTR_DF_NULO
        || toma->local == cntr_falso)
        return CNTR_ERROR;

    if (   (toma->gtls != NULL)
        && (   cntr_inicia_globalmente_capa_tls_servidor(toma->gtls)
            != CNTR_HECHO))
        return CNTR_ERROR;

    struct addrinfo *rp;

    for (rp = toma->infred; rp != NULL; rp = rp->ai_next) {
        /* Crear toma de entrada y guardar df asociado a ella */
        toma->servidor = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (toma->servidor == CNTR_DF_NULO)
            continue;
        /* Asociar toma de entrada a una dirección IP y un puerto */
        int activo = 1;
        setsockopt(toma->servidor, SOL_SOCKET, SO_REUSEADDR,
                   &activo, sizeof(activo));
        if (bind(toma->servidor, rp->ai_addr, rp->ai_addrlen) == 0)
            break; /* Hecho */
        close(toma->servidor);
    }

    if (rp == NULL) {
        perror("bind");
        return CNTR_ERROR;
    }

    /* Poner toma en modo escucha */
    if (listen(toma->servidor, CNTR_MAX_PENDIENTES) < 0) {
        perror("listen");
        return CNTR_ERROR;
    }

#if GNU_LINUX
    /* Sonda: df que hace referencia a la nueva instancia de epoll */
    toma->sonda->dfsd = epoll_create1(0);
    if (toma->sonda->dfsd == -1) {
        perror("epoll_create1");
        return CNTR_ERROR;
    }

    /* Incluir toma de escucha en la lista de interés */
    toma->sonda->evt->events = EPOLLIN;
    toma->sonda->evt->data->fd = toma->servidor;
    if (epoll_ctl(toma->sonda->dfsd, EPOLL_CTL_ADD, toma->servidor,
        &(toma->sonda->evt)) == -1) {
        perror("epoll_ctl");
        return CNTR_ERROR;
    }
#endif

    cntr_borra_infred(toma); /* Ya no se necesita */
    return CNTR_HECHO;
}

#if GNU_LINUX
/* cntr_trae_primer_cliente_toma */

int
cntr_trae_primer_cliente_toma(t_cntr_toma_es *toma, struct sockaddr *cliente)
{
    if (   toma == NULL
        || toma->servidor == CNTR_DF_NULO )
        return CNTR_ERROR;

    socklen_t lnt = (socklen_t) sizeof(*cliente);

    if (toma->sonda->ctdr < toma->sonda->ndsf) {
        toma->sonda->ctdr++;
        goto atiende_resto_eventos;
    }
    while(1) {
        /* Espera eventos en la instancia epoll refenciada en la sonda */
        toma->sonda->ndsf = epoll_wait(toma->sonda->dfsd, toma->sonda->eva,
                                       CNTR_MAX_EVENTOS, -1);
        if (toma->sonda->ndsf == -1) {
            perror("epoll_wait");
            return CNTR_ERROR;
        }
        for (toma->sonda->ctdr = 0; toma->sonda->ctdr < toma->sonda->ndsf;
             ++(toma->sonda->ctdr)) {
atiende_resto_eventos:
            if (   toma->sonda->eva[toma->sonda->ctdr].data.fd
                == toma->sonda->servidor) {
                /* Extraer primera conexión de la cola de conexiones */
                toma->cliente = accept(toma->servidor, cliente, &lnt);
                /* ¿Es cliente? */
                if (toma->cliente < 0) {
                    perror("accept");
                    return CNTR_ERROR;
                }
                /* Sí, es cliente */
                setnonblocking(toma->cliente);
                toma->sonda->evt->events = EPOLLIN | EPOLLOUT | EPOLLET;
                toma->sonda->evt->data->fd = toma->cliente;
                if (epoll_ctl(toma->sonda->dfsd, EPOLL_CTL_ADD, toma->cliente,
                              &(toma->sonda->evt)) == -1) {
                    perror("epoll_ctl");
                    return CNTR_ERROR;
                }
            } else {
                toma->cliente = toma->sonda->eva[toma->sonda->ctdr].data.fd;
                goto sal_y_usa_el_df;
            }
        }
    }
sal_y_usa_el_df:
    return CNTR_HECHO;
}
#else
/* cntr_trae_primer_cliente_toma */

int
cntr_trae_primer_cliente_toma(t_cntr_toma_es *toma, struct sockaddr *cliente)
{
    if (   toma == NULL
        || toma->servidor == CNTR_DF_NULO )
        return CNTR_ERROR;

    socklen_t lnt = (socklen_t) sizeof(*cliente);
    fd_set lst_df_sondear_lect, lst_df_sondear_escr;

    /* Borrar colección de tomas E/S a sondear */
    FD_ZERO(&lst_df_sondear_lect);
    FD_ZERO(&lst_df_sondear_escr);
    /* Sondear toma de escucha */
    FD_SET(toma->servidor, &lst_df_sondear_lect);

    while (1) {
        if (   (toma->gtls != NULL)
            && (   cntr_inicia_sesion_capa_tls_servidor(toma->gtls)
                != CNTR_HECHO))
            return CNTR_ERROR;

        /* Esperar a que los df estén listos para hacer operaciones de E/S */
        if (select(FD_SETSIZE, &lst_df_sondear_lect, &lst_df_sondear_escr,
                   NULL, NULL) < 0) {
            perror("select");
            return CNTR_ERROR;
        }
        /* Atender tomas con eventos de entrada pendientes */
        if (FD_ISSET(toma->servidor, &lst_df_sondear_lect)) {
            /* Extraer primera conexión de la cola de conexiones */
            toma->cliente = accept(toma->servidor, cliente, &lnt);
            /* ¿Es cliente? */
            if (toma->cliente < 0) {
                perror("accept");
                return CNTR_ERROR;
            }
            /* Sí, es cliente */
sondea_salida:
            FD_ZERO(&lst_df_sondear_lect);
            FD_ZERO(&lst_df_sondear_escr);
            FD_SET(toma->cliente, &lst_df_sondear_lect);
            FD_SET(toma->cliente, &lst_df_sondear_escr);
        } else {
            if (   FD_ISSET(toma->cliente, &lst_df_sondear_lect)
                && FD_ISSET(toma->cliente, &lst_df_sondear_escr))
                break;
            else
                goto sondea_salida;
        }
    }
    return CNTR_HECHO;
}
#endif

/* cntr_cierra_toma_cliente */

int
cntr_cierra_toma_cliente(t_cntr_toma_es *toma, int forzar)
{
    return cntr_cierra_toma(toma, toma->cliente, 1, forzar);
}

/* cntr_cierra_toma_servidor */

int
cntr_cierra_toma_servidor(t_cntr_toma_es *toma, int forzar)
{
    return cntr_cierra_toma(toma, toma->servidor, 0, forzar);
}

/* cntr_cierra_toma */

int
cntr_cierra_toma(t_cntr_toma_es *toma, int df_toma, int cliente, int forzar)
{
    /* Forzar cierre y evitar (TIME_WAIT) */
    if (forzar) {
        struct linger so_linger;
        so_linger.l_onoff  = 1;
        so_linger.l_linger = 0;
        setsockopt(df_toma, SOL_SOCKET, SO_LINGER,
                   &so_linger, sizeof(so_linger));
    }
    if (toma->gtls != NULL) {
        if (cntr_cierra_toma_tls(toma->gtls, cliente, df_toma) < 0) {
            return CNTR_ERROR;
        }
    } else {
        if (close(df_toma) < 0) {
            perror("cierra");
            return CNTR_ERROR;
        }
    }
    return CNTR_HECHO;
}