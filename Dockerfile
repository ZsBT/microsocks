FROM alpine:latest AS builder

RUN apk update
RUN apk add build-base

WORKDIR /usr/src
COPY . .
RUN make all


FROM alpine:latest AS flat

COPY --from=builder /usr/src/microsocks /usr/local/bin/microsocks

USER nobody

ENTRYPOINT [ "/usr/local/bin/microsocks" ]
STOPSIGNAL 9

LABEL org.label-schema.version='1.0'
LABEL org.label-schema.name='microsocks'
LABEL org.label-schema.description='multithreaded, small, efficient SOCKS5 server'
LABEL org.label-schema.url='https://github.com/rofl0r/microsocks'
LABEL org.label-schema.vcs-url='https://github.com/ZsBT/microsocks'
LABEL org.label-schema.docker.cmd='docker run -d -p 1080:1080'
LABEL org.label-schema.docker.cmd.help='docker exec -it $CONTAINER --help'

