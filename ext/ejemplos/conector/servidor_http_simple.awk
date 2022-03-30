#!/usr/bin/gawk -E

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

@load "conector";

BEGIN {
    # HTTP/1.1 define la secuencia <retorno de carro> \r <salto de línea> \n
    # como delimitador para todos los elementos, excepto en el cuerpo del
    # mensaje.
    RS = ORS = "\r\n";

    # Si el tope máximo (TMP) es 0 leeremos datos hasta RS, si es mayor que
    # cero leeremos una cantidad TPM de 'bytes' cada vez.
    TPM = 0;

    canalTcpIP = "/ired/tcp/" ARGV[1] "/" ARGV[2] "/0/0";
    creatoma(canalTcpIP);

    while (1) {
        print "[" PROCINFO["pid"] "]", "Esperando petición...";
        traepcli(canalTcpIP, cli);
        print "[" PROCINFO["pid"] "]", "Petición recibida desde " \
            cli["dir"] ", puerto " cli["pto"] ".";

        # Procesar petición
        salir = 0;
        while ((canalTcpIP |& getline) > 0) {
            print "<", $0;
            if ($1 == "GET" && $2 == "/salir")
                salir = 1;
            if (length($0) == 0)
                break;
        }

        # Mandar respuesta
        print "[" PROCINFO["pid"] "]",
              "Respuesta enviada hacia " \
              cli["dir"] ", puerto " cli["pto"] ".";
        print "> HTTP/1.1 200 Vale";
        print "HTTP/1.1 200 Vale" |& canalTcpIP;
        print "> Connection: close"
        print "Connection: close" |& canalTcpIP;

        acabacli(canalTcpIP);

        if (salir)
            break;
    }

    acabasrv(canalTcpIP);
    dtrytoma(canalTcpIP);

    exit 0;
}