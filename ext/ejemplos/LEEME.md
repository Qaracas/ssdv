# Cómo usar las extensiones de gawk leeflujo y conector

## Extensión leeflujo

Permite usar gawk para leer flujos de datos, en lugar de líneas delimitadas por la variable RS. Se
introduce la variable numérica TPM (ToPe Máximo) para fijar los bytes que se quieren leer cada vez.

### Ejemplo 1: leer fichero

Leer el flujo de datos proveniente de un fichero de *n* en *n* veces, siendo *n* el primer argumento
pasado por línea de comandos.

Ejecutando el ejemplo *lee_flujo.awk* de las siguiente forma:

```bash
(leeflujo)$ ./lee_flujo.awk lee_flujo.awk 3
```

se autoleerá a sí mismo imprimiendose por pantalla de 3 en 3 bytes.

Nuevas variables que añade leeflujo:

* TMP - Tope máximo o número máximo de bytes a leer
* LTD - Número de bytes realmente leídos

## Extensión conector

Permite a gawk comunicarse con otros procesos a través de TCP o TLS, mediante el uso de ficheros
especiales del estilo de los que gawk ya utiliza para
[programar en red](https://www.gnu.org/software/gawk/manual/html_node/TCP_002fIP-Networking.html).

### Ejemplo 1: servidor HTTP simple sobre TCP

En este ejemplo se ilustra cómo comunicar dos procesos a través de una conexión
[TCP](https://es.wikipedia.org/wiki/Protocolo_de_control_de_transmisi%C3%B3n).

Iniciar servidor HTTP en un terminal:

```bash
(conector)$ ./servidor_http_simple.awk localhost 7070
```

Conectar con servidor usando curl ([1]) desde otro terminal:

```bash
(conector)$ curl -v http://localhost:7070/prueba.html
```

Conectar y apagar el servidor desde otro terminal:

```bash
(conector)$ curl -v http://localhost:7070/salir
```

### Ejemplo 2: servidor HTTP ramificado sobre TCP

Se trata de una versión mejorada del ejemplo anterior, que incluye alta disponibilidad usando procesos
separados para atender cada conexión de los clientes.

### Ejemplo 3: servidor HTTP simple sobre TLS

Este ejemplo ilustra cómo usar la extensión conector utilizando la capa
[TLS](https://es.wikipedia.org/wiki/Seguridad_de_la_capa_de_transporte) para establecer conexiones
seguras y verificables entre cliente y servidor.

Iniciar servidor HTTPS (HTTP sobre TLS) en un terminal:

```bash
(conector)$ ./servidor_https_simple.awk localhost 7070
```

Para conectar con servidor es necesario verificar su identidad usando su certificado raiz. Esto se puede
hacer desde otro terminal usando curl ([2]), de la siguiente manera:

```bash
(conector)$ curl -v --cacert certificado_ac.pem https://localhost:7070/prueba.html
```

Conectar y apagar el servidor desde otro terminal:

```bash
(conector)$ curl -v --cacert certificado_ac.pem https://localhost:7070/salir
```

### Ejemplo 4: servidor HTTP de doble toma (una TCP y otra TLS)

En este caso se trata de un servidor que escucha por dos puertos al miesmo tiempo. Uno de los puertos
se le pasa por línea de comandos en el primer argumento, el otro en el segundo. El primero admite
conexiones TCP, y el segundo las admite TLS.

Iniciar servidor HTTP en un terminal:

```bash
(conector)$ ./servidor_http_doble_toma.awk 6060 7070
```

Conectar al puerto TCP del servidor usando curl desde otro terminal:

```bash
(conector)$ curl -v http://localhost:6060/salir.html
```

Conectar al puerto TLS del servidor usando curl desde otro terminal:

```bash
(conector)$ curl -v --cacert certificado_ac.pem https://localhost:7070/salir.html
```

Conectar y apagar el servidor desde otro terminal:

```bash
(conector)$ curl -v http://localhost:6060/salir
(conector)$ curl -v --cacert certificado_ac.pem https://localhost:7070/salir
```

# Cómo usar la herramienta certtool

Para ejecutar los programs de ejemplo en los cuales se usa la capa TLS
se necesita disponer de una clave privada, un certificado de servidor y,
adicionalmente, un fichero de estado del certificado.

Pinchar en los siguientes vínculos para obtener más información:

* [Cómo se usa certtool](https://www.gnutls.org/manual/html_node/certtool-Invocation.html)
* [Cómo se usa ocsptool](https://www.gnutls.org/manual/html_node/ocsptool-Invocation.html)
* [Solicitud de estado OCSP](https://www.gnutls.org/manual/html_node/OCSP-status-request.html#OCSP-status-request)

## Autoridad certificadora: generar clave privada

```bash
(conector)$ certtool --generate-privkey --outfile clave_privada_ac.pem
```

## Autoridad certificadora: generar certificado autofirmado

```bash
(conector)$ certtool --generate-self-signed --load-privkey clave_privada_ac.pem --outfile certificado_ac.pem
```

Normalmente este certificado pertenece a una autoridad certificadota que firma otros certificados

## Servidor: generar clave privada

```bash
(conector)$ certtool --generate-privkey --outfile clave_privada_servidor.pem
```

## Servidor: generar solicitud de certificado

```bash
(conector)$ certtool --generate-request --load-privkey clave_privada_servidor.pem --outfile solicitud_certificado_servidor.pem
```

## Servidor: generar certificado a partir de la solicitud

```bash
(conector)$ certtool --generate-certificate --load-request solicitud_certificado_servidor.pem \
   --outfile certificado_servidor.pem --load-ca-certificate certificado_ac.pem \
   --load-ca-privkey clave_privada_ac.pem
```

## Solicitud de estado OCSP (siglas en inglés de Protocolo de Estado de Certificado En línea)

El protocolo de estado de certificado en línea (OCSP) permite al cliente
verificar si el certificado del servidor sigue vigente sin necesidad de
comunicar con una lista de certificados revocados. El inconveniete es que el
cliente debe conectarse al OCSP del servidor, y solictar el estado del
certificado. Sin embargo, esta extensión permite a un servidor incluir la
respuesta OCSP durante el diálogo TLS. Es decir, un servidor puede ejecutar
periódicamente el comando
[ocsptool](https://www.gnutls.org/manual/html_node/ocsptool-Invocation.html)
para obtener la vigencia de su certificado y servirla a sus clientes. De esta
forma el cliente se evita conectar con un servidor OCSP externo.

[1]: https://curl.se/ "Biblioteca y herramienta en línea de comandos para transmitir datos con URLs"
[2]: https://curl.se/docs/sslcerts.html "Obtener certificado raiz que pueda verificar el servidor remoto"
