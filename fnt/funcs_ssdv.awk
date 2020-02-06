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

@include "bbl_jdva.awk";
@include "tipos_de_medios.awk";

# Listas internas:
#
# Índice extensiones ficheros y tipos de medios:
#
#   LSTTM[<extensión fichero>] = <tipo de medio> 
#       Ejem: LSTM["txt"] = "text/plain";
#
# Índice del contenido para cada dominio:
#
#   LSTDM[<dominio>][<ruta fichero>] = <tipo de medio>;
#
# Índice de acciones para cada estado HTTP:
#
#   LTEST[<código estado HTTP>]["funcion"] = "Envia_http";
#
# Índice dominios y rutas del contenido
#
#   LSTDR[<dominio>] = <ruta del contenido>;


# Ver:
# https://www.gnu.org/software/gawk/manual/html_node/Getopt-Function.html
#
# getopt.awk --- Do C library getopt(3) function in awk
#
# External variables:
#    Optind -- índice en ARGV del primer argumento sin opción
#    Optarg -- cadena con el argumento de la opción actual
#    Opterr -- si no es cero, pinta nuestro propio diagnóstico
#    Optopt -- letras de la opción actual
#
# Returns:
#    -1     al final de las opciones
#    "?"    para opciones desconocidas
#    <c>    caracter representando la opción actual
#
# Private Data:
#    _opti  -- index in multiflag option, e.g., -abc
#
function getopt(argc, argv, options,    thisopt, i)
{
    if (length(options) == 0)
        return -1;
    if (argv[Optind] == "--") {
        Optind++;
        _opti = 0;
        return -1;
    } else if (argv[Optind] !~ /^-[^:[:space:]]/) {
        _opti = 0;
        return -1;
    }
    if (_opti == 0)
        _opti = 2;
    thisopt = substr(argv[Optind], _opti, 1);
    Optopt = thisopt;
    i = index(options, thisopt);
    if (i == 0) {
        if (Opterr)
            printf("%c -- opción inválida\n", thisopt) > "/dev/stderr";
        if (_opti >= length(argv[Optind])) {
            Optind++;
            _opti = 0;
        } else
            _opti++;
        return "?";
    }
    if (substr(options, i + 1, 1) == ":") {
        # get option argument
        if (length(substr(argv[Optind], _opti + 1)) > 0)
            Optarg = substr(argv[Optind], _opti + 1);
        else
            Optarg = argv[++Optind];
        _opti = 0;
    } else
        Optarg = "";
    if (_opti == 0 || _opti >= length(argv[Optind])) {
        Optind++;
        _opti = 0;
    } else
        _opti++;
    return thisopt;
}

function Indexar_contenido_ssdv(dominio, contenido,      ruta, prcs)
{
    prcs = "ls -lR " contenido;
    while (prcs |& getline) {
        if ($0 !~ /(:$|^-[rwx-]{9})/) continue;
        if ($0 ~ /:$/) {
            gsub(/:/, "/", $0);
            gsub(contenido, "", $0);
            ruta = $0;
        } else {
            LSTDM[dominio][ruta $NF] = _trae_tipo_medio($NF);
        }
    }
}

function _trae_tipo_medio(fichero,      ext)
{
    ext = substr($NF, index($NF, ".") + 1, length($NF));
    if (ext in LSTTM)
        return LSTTM[ext];
    else
        return LSTTM["desconocido"];
}

function ProcesaOpciones_ssdv(argc, argv)
{
    while ((c = getopt(argc, argv, "c:")) != -1) {
        switch (c) {
        case "c":
            Opcn["ruta_conf"] = Optarg;
            break;
        case "?":
        default:
            Usar_ssdv();
            exit 1;
        }
    }
}

function LeeConfiguracion_ssdv(fichero_cfg, lst_cfg,      j, ln, r)
{
    j[0] = "";
    ln = "";
    
    TPM = 0; r = RS; RS = "\r?\n"
    while ((getline ln < fichero_cfg) > 0)
        j[0] = j[0] ln;
    close(fichero_cfg);
    RS = r;

    jsonLstm(j, lst_cfg); 
}

function Usar_ssdv(opcion)
{
    printf ("%s\n%s\n", \
        "opcion desconocida: " opcion \
        "usar: ssdc [-c <ruta>]") > "/dev/stderr";
}