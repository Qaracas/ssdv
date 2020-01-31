# Autor: Ulpiano Tur de Vargas <ulpiano.tur.devargas@gmail.com>
#
# Este programa es software libre; puedes distribuirlo y/o
# modificarlo bajo los términos de la Licencia Pública General de GNU
# según la publicó la Fundación del Software Libre; ya sea la versión 3, o
# (a su elección) una versión superior.
#
# Este programa se distribuye con la esperanza de que sea útil,
# pero SIN NINGUNA GARANTIA; ni siquiera la garantía implícita de
# COMERCIABILIDAD o APTITUD PARA UN PROPÓSITO DETERMINADO. Vea la
# Licencia Pública General de GNU para más detalles.
#
# Deberías haber recibido una copia de la Licencia Pública General
# de GNU junto con este software; mira el fichero LICENSE. Si
# no, mira <https://www.gnu.org/licenses/>.

# Author: Ulpiano Tur de Vargas <ulpiano.tur.devargas@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this software; see the file LICENSE. If
# not, see <https://www.gnu.org/licenses/>.

@include "funcs_util.awk";
@load "leeflujo"

BEGIN{
    CODALF = "charset=UTF-8";

    STD[200]["funcion"] = "EnviaTroceado_http";
    STD[404]["funcion"] = "Envia_http";
    STD[405]["funcion"] = "Envia_http";
}

function EnviaTxt_http(_texto, canalTcpIP)
{
    printf "%s", "Content-Length: " NumOctetos_util(_texto) CRLF CRLF |& canalTcpIP;
    printf "%s", _texto[0] |& canalTcpIP;
}

function Envia_http(fichero, canalTcpIP,      txt, r)
{
    txt[0] = "";
    TPM = 0; r = RS; RS = "\r?\n"
    while ((getline txt[0] < fichero) > 0) {
        txt[0] = txt[0] RS;
    }
    close(fichero);
    RS = r;
    EnviaTxt_http(txt, canalTcpIP)
}

function EnviaTroceado_http(fichero, canalTcpIP,      ln, hx, dc)
{
    ln[0] = "";
    printf "%s", "Transfer-Encoding: chunked" CRLF CRLF |& canalTcpIP;
    # (leeflujo) Leemos trozos de 512 octetos
    TPM = MAX;
    while ((getline ln[0] < fichero) > 0) {
        dc = NumOctetos_util(ln);
        hx = sprintf("%x", dc);
        printf "%s", hx CRLF ln[0] CRLF |& canalTcpIP;
    }
    close(fichero);
    TPM = 0;
    printf "%s", 0 CRLF CRLF |& canalTcpIP;
}

function IniciaRespuesta_http(LineaEstRespHttp, CabeceraResHttp, canalTcpIP,      i)
{
    printf "%s %s %s", LineaEstRespHttp["version"], \
                       LineaEstRespHttp["codigo"],  \
                       LineaEstRespHttp["texto"] CRLF |& canalTcpIP;
    for (i in CabeceraResHttp)
        printf "%s: %s", i, CabeceraResHttp[i] CRLF |& canalTcpIP;
}

function EsperaPeticion_http(canalTcpIP,      i)
{
    i = 0;
    while ((canalTcpIP |& getline) > 0) {
        if (i++ < 1) {
            LineaIniPetHttp["metodo"]   = $1;
            LineaIniPetHttp["objetivo"] = $2;
            LineaIniPetHttp["version"]  = $3;
        } else {
            if (! gsub(/:/, "", $1))
                break;
            CabeceraPetHttp[$1] = $2;
        }
    }
}

function Inicia_http()
{
    CRLF = RS = "\r\n";
    MAX  = 512;
    LineaIniPetHttp["version"]  = "HTTP/1.1";
    LineaEstRespHttp["version"] = "HTTP/1.1";
}