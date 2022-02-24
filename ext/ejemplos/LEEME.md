# Ejemplos de uso de la herramienta certtool

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

## Conectar con cualquiera de los servidores de ejemplo usado TSL

Consultar [verificar cerrificados SSL](https://curl.se/docs/sslcerts.html)
para más información.

