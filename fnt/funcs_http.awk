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
    VERSION = "HTTP/1.1";
    CODALF  = "charset=UTF-8";

    LTEST[200]["funcion"] = "";
    LTEST[200]["tpmedio"] = "";
    LTEST[200]["fichero"] = "";

    LTEST[404]["funcion"] = "Envia_http";
    LTEST[404]["tpmedio"] = "text/html";
    LTEST[404]["fichero"] = "./html/404.html";

    LTEST[405]["funcion"] = "Envia_http";
    LTEST[405]["tpmedio"] = "text/html";
    LTEST[405]["fichero"] = "./html/405.html";
}

function EnviaTxt_http(_texto, num_octetos, canalTcpIP)
{
    printf "%s", "Content-Length: " num_octetos CRLF CRLF \
        |& canalTcpIP;
    printf "%s", _texto[0] |& canalTcpIP;
}

function Envia_http(fichero, canalTcpIP,      ln, txt, i)
{
    txt[0] = "";
    TPM = MAX; i = 0;
    while ((getline ln < fichero) > 0) {
        txt[0] = txt[0] ln;
        i = i + LTD;
    }
    close(fichero);
    TPM = 0;
    EnviaTxt_http(txt, i, canalTcpIP)
}

function EnviaTroceado_http(fichero, canalTcpIP,      ln, hx)
{
    ln[0] = "";
    printf "%s", "Transfer-Encoding: chunked" CRLF CRLF |& canalTcpIP;
    # (leeflujo) Leemos trozos de 512 octetos
    TPM = MAX;
    while ((getline ln[0] < fichero) > 0) {
        hx = sprintf("%x", LTD);
        printf "%s", hx CRLF ln[0] CRLF |& canalTcpIP;
    }
    close(fichero);
    TPM = 0;
    printf "%s", 0 CRLF CRLF |& canalTcpIP;
}

function LeePeticion_http(canalTcpIP,      i)
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

function ProcesaPeticion_http(LineaIniPetHttp, CabeceraPetHttp,      dominio)
{
    dominio = CabeceraPetHttp["Host"];
    gsub(/:[0-9]+$/,"", dominio);

    print "[" PROCINFO["pid"] "]", "*", strftime();
    print "[" PROCINFO["pid"] "]", "*", CabeceraPetHttp["User-Agent"];
    print "[" PROCINFO["pid"] "]", "<", LineaIniPetHttp["version"], \
                LineaIniPetHttp["metodo"],                          \
                LineaIniPetHttp["objetivo"];

    if (LineaIniPetHttp["version"] == ""   \
        || LineaIniPetHttp["metodo"] == "" \
        || LineaIniPetHttp["objetivo"] == "") {
        LineaEstResHttp["codigo"] = 400;
        LineaEstResHttp["texto"]  = "Petición incorrecta";
        LTEST[400]["tpmedio"] = LTEST[400]["tpmedio"] ";" CODALF;
    }
    else if (LineaIniPetHttp["metodo"] != "GET") {
        LineaEstResHttp["codigo"] = 405;
        LineaEstResHttp["texto"]  = "Método no permitido";
        LTEST[405]["tpmedio"] = LTEST[405]["tpmedio"] ";" CODALF;
    }
    else if (   !length(LSTDM[dominio]) \
             || !(LineaIniPetHttp["objetivo"] in LSTDM[dominio])) {
        LineaEstResHttp["codigo"] = 404;
        LineaEstResHttp["texto"]  = "No Encontrado";
        LTEST[404]["tpmedio"] = LTEST[404]["tpmedio"] ";" CODALF;
    }
    else {
        LineaEstResHttp["codigo"] = 200;
        LineaEstResHttp["texto"]  = "Vale";

        LTEST[200]["tpmedio"] = LSTDM[dominio][LineaIniPetHttp["objetivo"]];
        LTEST[200]["fichero"] = LSTDR[dominio] LineaIniPetHttp["objetivo"];

        if (LTEST[200]["tpmedio"] ~ /^image/) {
            CabeceraResHttp["Accept-Ranges"] = "bytes";
            LTEST[200]["funcion"] = "Envia_http";
        } else {
            LTEST[200]["funcion"] = "EnviaTroceado_http";
            LTEST[200]["tpmedio"] = LTEST[200]["tpmedio"] ";" CODALF;
        }
    }
}

function EnviaRespuesta_http(LineaEstResHttp, canalTcpIP,      i, f, c)
{
    c = LineaEstResHttp["codigo"];

    print "[" PROCINFO["pid"] "]", ">", LineaEstResHttp["version"], c, \
                LineaEstResHttp["texto"];

    CabeceraResHttp["Server"] = "Servidor simple de Vargas/1.0";
    CabeceraResHttp["Content-Type"] = LTEST[c]["tpmedio"];

    printf "%s %s %s", LineaEstResHttp["version"], c, \
        LineaEstResHttp["texto"] CRLF |& canalTcpIP;

    for (i in CabeceraResHttp)
        printf "%s: %s", i, CabeceraResHttp[i] CRLF |& canalTcpIP;

    f = LTEST[c]["funcion"];
    @f(LTEST[c]["fichero"], canalTcpIP);
}

function Inicia_http()
{
    CRLF = RS = "\r\n";
    MAX  = 256;
    LineaIniPetHttp["version"] = VERSION;
    LineaEstResHttp["version"] = VERSION;
}
