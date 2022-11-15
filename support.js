var counter = 0;

function gensym (s) {
    counter += 1;
    return s + counter;
}
