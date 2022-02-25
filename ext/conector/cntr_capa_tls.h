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

#ifndef CAPA_TLS_H
#define CAPA_TLS_H

#define VERIFICA_ERROR(valret, cmd, cmdtxt) \
        if ((valret = cmd) < GNUTLS_E_SUCCESS) { \
            fprintf(stderr, \
                    cmdtxt, \
                    gnutls_strerror(valret)); \
            return CNTR_ERROR; \
        }

#define BUCLE_VERIFICA(valret, cmd) \
        do { \
                valret = cmd; \
        } while(valret == GNUTLS_E_AGAIN || valret == GNUTLS_E_INTERRUPTED)

struct gnutls_session_int;
typedef struct gnutls_session_int *gnutls_session_t;

struct gnutls_certificate_credentials_st;
typedef struct gnutls_certificate_credentials_st 
*gnutls_certificate_credentials_t;

struct gnutls_priority_st;
typedef struct gnutls_priority_st *gnutls_priority_t;

#ifndef T_CTRN_VERDAD
#define T_CTRN_VERDAD
typedef enum cntr_verdad {
    cntr_falso  = 0,
    cntr_cierto = 1
} t_ctrn_verdad;
#endif

typedef struct capa_gnutls {
    gnutls_certificate_credentials_t credx509;  /* Est. certificado X.509 */
    gnutls_priority_t                prioridad; /* Para cifrado y claves  */
    gnutls_session_t                 sesión;    /* Sesión TLS             */
    t_ctrn_verdad                    sesión_iniciada : 1;
    t_ctrn_verdad                    usándose : 1;
} t_capa_gnutls;


/* cntr_inicia_globalmente_capa_tls_servidor --
 *
 * Inicializa parámetros globales de la capa TLS en toma local de escucha
 */

int 
cntr_inicia_globalmente_capa_tls_servidor(t_capa_gnutls *capatls);


/* cntr_inicia_sesion_capa_tls_servidor --
 *
 * Inicia sesión TLS en toma local de escucha
 */

int
cntr_inicia_sesion_capa_tls_servidor(t_capa_gnutls *capatls);

/* cntr_finaliza_sesion_capa_tls --
 *
 * Borra la sesión y los topes que tiene asociados
 */

void
cntr_finaliza_sesion_capa_tls(t_capa_gnutls *capatls);

/* cntr_envia_datos_capa_tls --
 *
 * Envía datos cifrados a traves de la capa TLS
 */

ssize_t
cntr_envia_datos_capa_tls(t_capa_gnutls *capatls, int df_cliente,
                          const void *tope, size_t bulto);

/* cntr_recibe_datos_capa_tls --
 *
 * Recibe datos descifrados de la capa TLS
 */

ssize_t
cntr_recibe_datos_capa_tls(t_capa_gnutls *capatls, int df_cliente, void *tope,
                           size_t bulto);

/* cntr_liberta_capa_toma_tls --
 *
 *
 */

void
cntr_liberta_capa_toma_tls(t_capa_gnutls *capatls);

/* cntr_cierra_toma_tls --
 *
 * Termina la conexión TLS del cliente o del servidor
 */

int
cntr_cierra_toma_tls(t_capa_gnutls *capatls, int cliente, int df);

/* cntr_par_clave_privada_y_certificado_tls --
 *
 *
 */

int
cntr_par_clave_privada_y_certificado_tls(t_capa_gnutls *capatls,
                                         const char *fclave,
                                         const char *fcertificado);

/* cntr_fichero_autoridades_certificadoras_tls --
 *
 * Carga las atoridades certificadores de confianza presentes en el
 * fichero para verificar certificados de cliente o de servidor. Se
 * puede llamar una o más veces.
 */

int
cntr_fichero_autoridades_certificadoras_tls(t_capa_gnutls *capatls,
                                            const char *fautoridades);

#endif /* CAPA_TLS_H */