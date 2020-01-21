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

BEGIN{
    CRLF = "\r\n";
    MAX  = 512;
    
    STD[200]["funcion"] = "envia_fichero_troceado";
    STD[404]["funcion"] = "envia_fichero";
    STD[405]["funcion"] = "envia_fichero";
}

function envia_texto(_texto, canalTcpIP)
{
    printf "%s", "Content-Length: " num_octetos(_texto) CRLF CRLF |& canalTcpIP;
    printf "%s", _texto[0] |& canalTcpIP;
}

function envia_fichero(fichero, canalTcpIP,      txt)
{
    txt[0] = "";
    while ((getline txt[0] < fichero) > 0) {
        txt[0] = txt[0] RS;
    }
    close(fichero);
    envia_texto(txt, canalTcpIP)
}

function envia_fichero_troceado(fichero, canalTcpIP,      ln, hx, dc, tp)
{
    ln[0] = "";
    printf "%s", "Transfer-Encoding: chunked" CRLF CRLF |& canalTcpIP;
    while ((getline ln[0] < fichero) > 0) {
        tp[0] = tp[0] ln[0] RS;
        dc = num_octetos(tp);
        hx = sprintf("%x", dc);
        if (dc > MAX) {
            printf "%s", hx CRLF tp[0] CRLF |& canalTcpIP;
            tp[0] = "";
        }
    }
    if (length(tp[0]))
        printf "%s", hx CRLF tp[0] CRLF |& canalTcpIP;
    close(fichero);
    printf "%s", 0 CRLF CRLF |& canalTcpIP;
}
