@0xcb1741e077a5bfd5;

struct GetMessageCP {
    key @0 :Text;
}

struct PutMessageCP {
    key @0 :Text;
    value @1 :Text;
}

struct Msg1LCP {
    left @0 :GetMessageCP;
    right @1 :GetMessageCP;
}

struct Msg2LCP {
    left @0 :Msg1LCP;
    right @1 :Msg1LCP;
}

struct Msg3LCP {
    left @0 :Msg2LCP;
    right @1 :Msg2LCP;
}

struct Msg4LCP {
    left @0 :Msg3LCP;
    right @1 :Msg3LCP;
}

struct Msg5LCP {
    left @0 :Msg5LCP;
    right @1 :Msg5LCP;
}

