FROM gcc:7.5

RUN apt-get update -y && apt-get install  -y libmicrohttpd-dev libjson-c-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . /api

WORKDIR /api

RUN gcc -o api api.c -ljson-c  -lmicrohttpd

EXPOSE 8888

CMD ["sh","-c","./api 8888"]
