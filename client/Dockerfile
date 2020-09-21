# Base image
FROM python:3

RUN mkdir /app

COPY index.html /app

WORKDIR /app

EXPOSE 9000

CMD [ "sh","-c","python -m http.server 9000"]
