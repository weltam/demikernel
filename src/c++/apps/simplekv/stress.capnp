@0xcb1741e077a5bfd5;

struct GetMessageCP {
    reqId @0 :Int32;
    key @1 :Text;
}

struct PutMessageCP {
    reqId @0 :Int32;
    key @1 :Text;
    value @2 :Text;
}


struct ResponseCP {
    reqId @0 :Int32;
    value @1 :Text;
}

