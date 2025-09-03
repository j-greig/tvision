static const long cubeX[] = {
    TO_FIX(-1.0), TO_FIX(-1.0), TO_FIX(-1.0), 
    TO_FIX(-1.0), TO_FIX(1.0), TO_FIX(1.0), 
    TO_FIX(1.0), TO_FIX(1.0), };
static const long cubeY[] = {
    TO_FIX(-1.0), TO_FIX(-1.0), TO_FIX(1.0), 
    TO_FIX(1.0), TO_FIX(-1.0), TO_FIX(-1.0), 
    TO_FIX(1.0), TO_FIX(1.0), };
static const long cubeZ[] = {
    TO_FIX(-1.0), TO_FIX(1.0), TO_FIX(-1.0), 
    TO_FIX(1.0), TO_FIX(-1.0), TO_FIX(1.0), 
    TO_FIX(-1.0), TO_FIX(1.0), };
static const int cubeEdges[] = {
2, 0, 0, 1, 1, 3, 
    3, 2, 6, 2, 3, 7, 
    7, 6, 4, 6, 7, 5, 
    5, 4, 0, 4, 5, 1, 
    };
const size_t cubeVtx=8;
const size_t cubeEdge=24;
