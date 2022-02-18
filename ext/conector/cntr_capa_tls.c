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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>

#include "cntr_defcom.h"
#include "cntr_capa_tls.h"

/* cntr_inicia_globalmente_capa_tls_servidor */

int
cntr_inicia_globalmente_capa_tls_servidor(t_capa_gnutls *capatls)
{
    int error;

    VRFC_ERROR(error, gnutls_global_init(),
               "gnutls_global_init(): %s\n");

    VRFC_ERROR(error,
               gnutls_certificate_allocate_credentials(&(capatls->credx509)),
               "gnutls_certificate_allocate_credentials(): %s\n");

    /* Prioridad de los cifrados y métodos de intercambio de claves */
    VRFC_ERROR(error,
               gnutls_priority_init(&(capatls->prioridad), NULL, NULL),
               "gnutls_priority_init(): %s\n");

    /* Disponible desde GnuTLS 3.5.6. En versiones anteriores consultar:
     * gnutls_certificate_set_dh_params() */
#if GNUTLS_VERSION_NUMBER >= 0x030506
    /* Configura parámetros Diffie-Hellman para que los use un servidor 
       con certificado*/
    gnutls_certificate_set_known_dh_params(capatls->credx509,
                                           GNUTLS_SEC_PARAM_MEDIUM);
#endif
    return CNTR_HECHO;
}

/* cntr_inicia_sesion_capa_tls_servidor */

int
cntr_inicia_sesion_capa_tls_servidor(t_capa_gnutls *capatls)
{
    int error;

    VRFC_ERROR(error,
               gnutls_init(&(capatls->sesión), GNUTLS_SERVER),
               "gnutls_init(): %s\n");

    /* Prioridad de los métodos de cifrado e intercambio de claves */
    VRFC_ERROR(error,
               gnutls_priority_set(capatls->sesión, capatls->prioridad),
               "gnutls_priority_set(): %s\n");

    VRFC_ERROR(error,
               gnutls_credentials_set(capatls->sesión, GNUTLS_CRD_CERTIFICATE,
                                      capatls->credx509),
               "gnutls_credentials_set(): %s\n");

    /* No solicitar ningún certificado al cliente */
    gnutls_certificate_server_set_request(capatls->sesión,
                                          GNUTLS_CERT_IGNORE);
    gnutls_handshake_set_timeout(capatls->sesión,
                                 GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

    return CNTR_HECHO;
}

/* cntr_finaliza_sesion_capa_tls */

void
cntr_finaliza_sesion_capa_tls(t_capa_gnutls *capatls)
{
    /* Borra la sesión y los topes que tiene asociados */
    gnutls_deinit(capatls->sesión);
}

/* cntr_envia_datos_capa_tls */

ssize_t
cntr_envia_datos_capa_tls(t_capa_gnutls *capatls, int df_cliente,
                          const void *tope, size_t bulto)
{
    (void) df_cliente;
    return gnutls_record_send(capatls->sesión, tope, bulto);
}

/* cntr_recibe_datos_capa_tls */

ssize_t
cntr_recibe_datos_capa_tls(t_capa_gnutls *capatls, int df_cliente, void *tope,
                           size_t bulto)
{
    int resul;
    /* Asocia nueva toma del cliente a la sesión TLS */
    gnutls_transport_set_int(capatls->sesión, df_cliente);

    /* Dialoga TLS e inicializa los parámetros de sesión */
    BUCLE_VERIFICA(resul, gnutls_handshake(capatls->sesión));
    if (resul < 0) {
        close(df_cliente);
        gnutls_deinit(capatls->sesión);
        fprintf(stderr,
                "Dialogo TLS fallido: %s\n",
                gnutls_strerror(resul));
        return CNTR_REINTENTAR;
    }

    BUCLE_VERIFICA(resul, gnutls_record_recv(capatls->sesión, tope, bulto));
    if (resul < 0) {
        fprintf(stderr,
                "gnutls_record_recv(): %s\n",
                gnutls_strerror(resul));
         return CNTR_ERROR;
    }

    return resul;
}

/* cntr_liberta_capa_toma_tls */

void
cntr_liberta_capa_toma_tls(t_capa_gnutls *capatls)
{
    if (capatls->usándose) {
        gnutls_certificate_free_credentials(capatls->credx509);
        gnutls_priority_deinit(capatls->prioridad);
        gnutls_global_deinit();
        capatls->usándose = 0;
    }
}

/* cntr_cierra_toma_tls */

int
cntr_cierra_toma_tls(t_capa_gnutls *capatls, int cliente, int df_toma)
{
    if (cliente) {
        gnutls_bye(capatls->sesión, GNUTLS_SHUT_WR);
        if (close(df_toma) < 0) {
            perror("close");
            return CNTR_ERROR;
        }
        gnutls_deinit(capatls->sesión);
    } else {
        if (close(df_toma) < 0) {
            perror("close");
            return CNTR_ERROR;
        }
        cntr_liberta_capa_toma_tls(capatls);
    }
    return CNTR_HECHO;
}

/* cntr_par_clave_privada_y_certificado_tls */

int
cntr_par_clave_privada_y_certificado_tls(t_capa_gnutls *capatls,
                                         const char *fcertificado,
                                         const char *fclave)
{
    int error;
    VRFC_ERROR(error,
               gnutls_certificate_set_x509_key_file(capatls->credx509,
                                                    fcertificado,
                                                    fclave,
                                                    GNUTLS_X509_FMT_PEM),
               "gnutls_certificate_set_x509_key_file(): %s\n");
    return CNTR_HECHO;
}

/* cntr_fichero_autoridades_certificadoras_tls */

int
cntr_fichero_autoridades_certificadoras_tls(t_capa_gnutls *capatls,
                                            const char *fautoridades)
{
    int error;
    VRFC_ERROR(error,
               gnutls_certificate_set_x509_trust_file(capatls->credx509,
                                                      fautoridades,
                                                      GNUTLS_X509_FMT_PEM),
               "gnutls_certificate_set_x509_trust_file(): %s\n");
    return CNTR_HECHO;
}