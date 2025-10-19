FROM alpine:latest as build
RUN apk update && apk add --no-cache g++ make
WORKDIR /app
COPY mlq_scheduler.cpp .
RUN g++ -o mlq_scheduler mlq_scheduler.cpp -static -static-libstdc++

FROM debian:bullseye-slim
WORKDIR /app
COPY --from=build /app/mlq_scheduler .
RUN chmod +x mlq_scheduler

CMD ["./mlq_scheduler"]
