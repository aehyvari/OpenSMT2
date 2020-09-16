//
// Created by prova on 02.09.20.
//
#include "IteToSwitch.h"
#include <fstream>

void IteToSwitch::printDagToFile(const std::string &fname, const ite::Dag &dag) {
    std::fstream fs;
    fs.open(fname, std::fstream::out);
    dag.writeDagToStream(fs);
    fs.close();
}

std::pair<int, int> ite::Dag::getNumBooleanItes(PTRef root, const Logic& logic) const {
    int num_bool_ites = 0;
    int num_total_ites = 0;
    vec<NodeRef> queue;
    queue.push(nodes[root]);
    vec<type> flag;
    flag.growTo(na.getNumNodes());

    while (queue.size() != 0) {
        NodeRef nr = queue.last();
        const Node& n = na[nr];
        if (flag[n.getId()] == type::black) {
            queue.pop();
            continue;
        }

        bool unprocessed_children = false;

        for (auto ch : {n.getFalseChild(), n.getTrueChild()}) {
            if (ch == NodeRef_Undef) {
                continue; // could be break as well.
            }
            const Node &c = na[ch];
            if (flag[c.getId()] == type::white) {
                queue.push(ch);
                flag[c.getId()] = type::gray;
                unprocessed_children = true;
            }
        }
        if (unprocessed_children) {
            continue;
        }

        flag[n.getId()] = type::black;
        if (!n.isLeaf() && logic.hasSortBool(n.getTerm())) {
            num_bool_ites++;
        } else {
            num_total_ites++;
        }
    }
    return {num_bool_ites, num_total_ites};
}

void ite::Dag::writeDagToStream(std::ostream &out) const {
    std::string annotations_str;
    std::string edges_str;
    auto &nodes = getNodes();
    std::cout << "Starting production of a graph" << std::endl;
    for (const ite::NodeRef node : nodes) {
        if (isTopLevelIte(na[node].getTerm())) {
            annotations_str += " " + std::to_string(na[node].getId()) + " [shape=box];\n";
        }
        if (na[node].getTrueChild() != NodeRef_Undef) {
            edges_str += " " + std::to_string(na[node].getId()) + " -> " + std::to_string(na[na[node].getTrueChild()].getId()) + ";\n";
        }
        if (na[node].getFalseChild() != NodeRef_Undef) {
            edges_str += " " + std::to_string(na[node].getId()) + " -> " + std::to_string(na[na[node].getFalseChild()].getId()) + ";\n";
        }
    }
    out << "digraph G {" << annotations_str << "\n" << edges_str << "}" << std::endl;
}