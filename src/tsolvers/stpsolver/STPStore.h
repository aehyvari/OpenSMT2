#ifndef OPENSMT_STPSTORE_H
#define OPENSMT_STPSTORE_H

#include <cstdint>
#include <vector>
#include <Number.h>
#include <SolverTypes.h>

struct VertexRef {
    uint32_t x;
    inline bool operator==(VertexRef other) const { return x == other.x; }
    inline bool operator!=(VertexRef other) const { return x != other.x; }
    explicit operator bool() const { return x != INT32_MAX; }
};

struct EdgeRef {
    uint32_t x;
    inline bool operator==(EdgeRef other) const { return x == other.x; }
    inline bool operator!=(EdgeRef other) const { return x != other.x; }
    explicit operator bool() const { return x != INT32_MAX; }
};

static VertexRef VertRef_Undef = VertexRef { INT32_MAX };
static EdgeRef EdgeRef_Undef = EdgeRef { INT32_MAX };

struct Edge {
    VertexRef from, to;
    EdgeRef neg;
    opensmt::Number cost;
};

class STPStore {
private:
    std::vector<VertexRef> verts;           // list of all created vertices
    std::vector<Edge> edges;                //                     edges
public:
    VertexRef createVertex();
    EdgeRef createEdge(VertexRef from, VertexRef to, opensmt::Number cost);

    size_t vertexNum() const  { return verts.size(); }
    size_t edgeNum() const { return edges.size(); }
    const Edge & getEdge(EdgeRef e) const { return edges[e.x]; }
    void setNegation(EdgeRef a, EdgeRef b);
};

#endif //OPENSMT_STPSTORE_H