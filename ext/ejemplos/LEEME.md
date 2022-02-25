# Ejemplos de cómo usar las extensiones de gawk leeflujo y conector

## Extensión leeflujo

Permite usar gawk para leer flujos de datos en lugar de líneas delimitadas por la variable RS. En su
lugar se introduce la variable numérica TPM (ToPe Máximo) para fijar los bytes que se quieren leer de
un golpe.

Ejecutando el ejemplo lee_flujo.awk de las siguiente forma:

```bash
(leeflujo)$ ./lee_flujo.awk lee_flujo.awk 3
```

se autoleerá a sí mismo imprimiendose por pantalla de 3 3n 3 bytes.

Nuevas variables que añade leeflujo:

* TMP - Tope máximo o número máximo de bytes a leer
* LTD - Número de bytes realmente leídos

## Extensión conector

Algunos ejemplos de uso de la extensión conector utilizan la capa TLS para establecer conexiones seguras
y verificables entre un cliente y un servidor.

### Servidor HTTP simple sobre TCP

Iniciar servidor HTTP:

```bash
(conector)$ ./servidor_http_simple.awk localhost 7070
```

Conectar con servidor usando [curl] [1]:

```bash
(conector)$ curl -v http://localhost:7070/prueba.html
```

Conectar y apagar el servidor:

```bash
(conector)$ curl -v http://localhost:7070/salir
```

### Servidor HTTP simple sobre TLS

Iniciar servidor HTTPS (HTTP sobre TLS)

```bash
(conector)$ ./servidor_https_simple.awk localhost 7070
```

Para conectar con servidor es necesario verificar su identidad usando su certificado raiz. Esto se puede
hacer con [curl] [2] de la siguiente manera:

```bash
(conector)$ curl -v --cacert certificado_servidor.pem https://localhost:7070/salir.html
```

Apagar servidor:

```bash
curl -v --cacert conector/certificado_servidor.pem https://localhost:7070/salir
```

# Cómo usar la herramienta certtool

Para ejecutar los programs de ejemplo en los cuales se usa la capa TLS
se necesita disponer de una clave privada, un certificado de servidor y,
adicionalmente, un fichero de estado del certificado.

Pinchar en los siguientes vínculos para obtener más información:

* [Cómo se usa certtool](https://www.gnutls.org/manual/html_node/certtool-Invocation.html)
* [Cómo se usa ocsptool](https://www.gnutls.org/manual/html_node/ocsptool-Invocation.html)
* [Solicitud de estado OCSP](https://www.gnutls.org/manual/html_node/OCSP-status-request.html#OCSP-status-request)

## Generar una clave privada

```bash
certtool --generate-privkey --outfile clave_privada.pem
```

## Generar un certificado autofirmado de servidor

```bash
certtool --generate-self-signed --load-privkey clave_privada.pem --outfile certificado_servidor.pem
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

[1]: https://curl.se/ "Biblioteca y herramienta de línea de comandos para transmitir datos con URLs"
[2]: https://curl.se/docs/sslcerts.html "Obtener certificado raiz que pueda verificar el servidor remoto"
