FROM alpine:latest AS builder

RUN apk update
RUN apk add build-base

WORKDIR /usr/src
COPY . .
ENV LDFLAGS=-static
RUN make


FROM scratch

ARG GITHUB_SERVER_URL
ARG GITHUB_REPOSITORY
ARG BUILD_DATE
ARG VCS_REF

COPY --from=builder /usr/src/microsocks /usr/local/bin/microsocks
COPY --from=builder /etc/passwd /etc/passwd
COPY --from=builder /etc/group /etc/group

USER nobody

EXPOSE 1080

ENTRYPOINT [ "/usr/local/bin/microsocks" ]

LABEL org.label-schema.version='1.6'
LABEL org.label-schema.name='microsocks'
LABEL org.label-schema.description='multithreaded, small, efficient SOCKS5 server'
LABEL org.label-schema.url="${GITHUB_SERVER_URL}/${GITHUB_REPOSITORY}"
LABEL org.label-schema.vcs-url="${GITHUB_SERVER_URL}/${GITHUB_REPOSITORY}"
LABEL org.label-schema.created="${BUILD_DATE}"
LABEL org.label-schema.vcs-ref="${VCS_REF}"
LABEL org.label-schema.docker.cmd='docker run -d -p 1080:1080'
LABEL org.label-schema.docker.cmd.help='docker exec -it $CONTAINER --help'
