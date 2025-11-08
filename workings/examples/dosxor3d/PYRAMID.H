static const long pyramidX[] = {
    TO_FIX(-1.0), TO_FIX(0.0), TO_FIX(-1.0), 
    TO_FIX(0.0), TO_FIX(1.0), TO_FIX(0.0), 
    TO_FIX(1.0), TO_FIX(0.0), TO_FIX(-1.0), 
    TO_FIX(-1.0), TO_FIX(1.0), TO_FIX(1.0), 
    };
static const long pyramidY[] = {
    TO_FIX(-1.0), TO_FIX(0.0), TO_FIX(1.0), 
    TO_FIX(0.0), TO_FIX(-1.0), TO_FIX(0.0), 
    TO_FIX(1.0), TO_FIX(0.0), TO_FIX(-1.0), 
    TO_FIX(1.0), TO_FIX(-1.0), TO_FIX(1.0), 
    };
static const long pyramidZ[] = {
    TO_FIX(-1.0), TO_FIX(1.0), TO_FIX(-1.0), 
    TO_FIX(1.0), TO_FIX(-1.0), TO_FIX(1.0), 
    TO_FIX(-1.0), TO_FIX(1.0), TO_FIX(3.0), 
    TO_FIX(3.0), TO_FIX(3.0), TO_FIX(3.0), 
    };
static const int pyramidEdges[] = {
2, 0, 0, 1, 1, 3, 
    3, 2, 6, 2, 3, 7, 
    7, 6, 4, 6, 7, 5, 
    5, 4, 0, 4, 5, 1, 
    8, 9, 9, 11, 11, 10, 
    10, 8, 3, 9, 11, 7, 
    10, 5, 1, 8, };

const size_t pyramidVtx=12;
const size_t pyramidEdge=40;
