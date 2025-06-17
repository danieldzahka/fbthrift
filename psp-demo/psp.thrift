namespace cpp2 example.psp

struct EchoRequest {
  1: i32 num;
}

struct EchoResponse {
  1: i32 num;
}

service EchoService {
  EchoResponse echo(1: EchoRequest request);
}
