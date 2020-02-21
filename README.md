# El servidor simple de Vargas (ssdv)

## ¿Qué es `ssdv`?

`ssdv` es un servidor [HTTP](https://es.wikipedia.org/wiki/Protocolo_de_transferencia_de_hipertexto) escrito en [AWK](https://www.gnu.org/software/gawk/manual/gawk.html) siguiendo un modelo concurrente.

## Requisitos

* Núcleo Linux.
* Versión de `GNU Awk`, `API 2.0`.
* Última versión de [jdva](https://github.com/Qaracas/jdva).

## Instalación

1. Instalar [jdva](https://github.com/Qaracas/jdva).

2. Descargar el proyecto completo escribiendo:

```bash
$ git clone git://github.com/Qaracas/ssdv.git
```

3. Fijar la variable de entorno [AWKPATH](https://www.gnu.org/software/gawk/manual/gawk.html#AWKPATH-Variable):

```bash
(ssdv) export AWKPATH=${AWKPATH}:"$(pwd)""/fnt"
```

4. Fijar la variable de entorno [AWKLIBPATH](https://www.gnu.org/software/gawk/manual/html_node/AWKLIBPATH-Variable.html)

```bash
(ssdv) export AWKLIBPATH=${AWKLIBPATH}:"$(pwd)""/ext"
```

## Referencias

* [Protocolo HTTP - Documentación de varias especificaciones](https://httpwg.org/specs/)
* [Protocolo HTTP - Estándar HTTP/1.1](https://tools.ietf.org/html/rfc2616)
* [Protocolo HTTP - Estándar HTTP/2](https://tools.ietf.org/html/rfc7540)
* [Protocolo HTTP - Artículo de la Wikipedia](https://es.wikipedia.org/wiki/Protocolo_de_transferencia_de_hipertexto)
* [Protocolo HTTP - Documentación para desarrolladores (Mozilla)](https://developer.mozilla.org/es/docs/Web/HTTP)
* [Protocolo HTTP - Resumen códigos de estado (W3C)](https://www.w3schools.com/tags/ref_httpmessages.asp)
* [Servidores HTTP - 3 técnicas de concurrencia](https://www.linuxjournal.com/content/three-ways-web-server-concurrency)
* [Servidores HTTP - Programación concurrente para arquitectura web escalable](https://oparu.uni-ulm.de/xmlui/bitstream/handle/123456789/2450/vts_8082_11772.pdf?sequence=1&isAllowed=y)

## Autores

* [Ulpiano Tur de Vargas](https://github.com/Qaracas)
* [Y Cía](https://github.com/Qaracas/ssdv/contributors)

## Licencia

Este proyecto se distribuye bajo los términos de la Licencia Pública General de GNU (GNU GPL v3.0). Mira el archivo [LICENSE](LICENSE) para más detalle.
