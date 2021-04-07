#ifndef DB_DIALECTS_QUERYGRAPHBUILDER_H
#define DB_DIALECTS_QUERYGRAPHBUILDER_H
namespace mlir::relalg {
class QueryGraphBuilder {
   using attribute_set = llvm::SmallPtrSet<mlir::relalg::RelationalAttribute*, 8>;

   Operator root;
   std::unordered_set<mlir::Operation*>& already_optimized;
   size_t num_nodes;
   QueryGraph qg;
   node_set empty_node;

   class NodeResolver {
      QueryGraph& qg;

      std::unordered_map<relalg::RelationalAttribute*, size_t> attr_to_nodes;

      public:
      explicit NodeResolver(QueryGraph& qg) : qg(qg) {}

      void add(relalg::RelationalAttribute* attr, size_t nodeid) {
         attr_to_nodes[attr] = nodeid;
      }

      void merge(const NodeResolver& other) {
         for (auto x : other.attr_to_nodes) {
            auto [attr, nodeid] = x;
            if (attr_to_nodes.count(attr)) {
               auto currid = attr_to_nodes[attr];
               if (qg.nodes[nodeid].op->isBeforeInBlock(qg.nodes[currid].op.getOperation())) {
                  currid = nodeid;
               }
               attr_to_nodes[attr] = currid;
            } else {
               attr_to_nodes[attr] = nodeid;
            }
         }
      }

      size_t resolve(relalg::RelationalAttribute* attr) {
         return attr_to_nodes[attr];
      }
   };
   size_t addNode(Operator op) {
      QueryGraph::Node n(op);
      n.id = qg.nodes.size();
      qg.nodes.push_back(n);
      node_for_op[op.getOperation()] = n.id;
      return n.id;
   }

   size_t countCreatingOperators(Operator op) {
      size_t res = 0;
      auto children = op.getChildren();
      auto used = op.getUsedAttributes();
      auto created = op.getCreatedAttributes();
      if (already_optimized.count(op.getOperation())) {
         res += 1;
         return res;
      }
      for (auto child : children) {
         res += countCreatingOperators(child);
      }
      if (mlir::isa<Join>(op.getOperation())) {
         if (!created.empty()) {
            res += 1;
         }
      } else if (!created.empty() && !mlir::isa<mlir::relalg::CrossProductOp>(op.getOperation())) {
         res += 1;
      }
      return res;
   }
   enum OpType {
      None,
      CP = 1,
      InnerJoin,
      SemiJoin,
      AntiSemiJoin,
      OuterJoin,
      FullOuterJoin,
      LAST
   };
   // @formatter:off
   // clang-format off
   bool assoc[OpType::LAST][OpType::LAST] = {
      /* None =  */{},
      /* CP           =  */{/*None=*/false,/*CP=*/true,/*InnerJoin=*/true,/*SemiJoin=*/true,/*AntiSemiJoin=*/true,/*OuterJoin=*/true,/*FullOuterJoin=*/false},
      /* InnerJoin    =  */{/*None=*/false,/*CP=*/true,/*InnerJoin=*/true,/*SemiJoin=*/true,/*AntiSemiJoin=*/true,/*OuterJoin=*/true,/*FullOuterJoin=*/false},
      /* SemiJoin     =  */{/*None=*/false,/*CP=*/false,/*InnerJoin=*/false,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false},
      /* AntiSemiJoin =  */{/*None=*/false,/*CP=*/false,/*InnerJoin=*/false,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false},
      /* OuterJoin    =  */{/*None=*/false,/*CP=*/false,/*InnerJoin=*/false,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false},
      /* FullOuterJoin=  */{/*None=*/false,/*CP=*/false,/*InnerJoin=*/false,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false}
   };
   bool l_asscom[OpType::LAST][OpType::LAST] = {
      /* None =  */{},
      /* CP           =  */{/*None=*/false,/*CP=*/true,/*InnerJoin=*/true,/*SemiJoin=*/true,/*AntiSemiJoin=*/true,/*OuterJoin=*/true,/*FullOuterJoin=*/false},
      /* InnerJoin    =  */{/*None=*/false,/*CP=*/true,/*InnerJoin=*/true,/*SemiJoin=*/true,/*AntiSemiJoin=*/true,/*OuterJoin=*/true,/*FullOuterJoin=*/false},
      /* SemiJoin     =  */{/*None=*/false,/*CP=*/true,/*InnerJoin=*/true,/*SemiJoin=*/true,/*AntiSemiJoin=*/true,/*OuterJoin=*/true,/*FullOuterJoin=*/false},
      /* AntiSemiJoin =  */{/*None=*/false,/*CP=*/true,/*InnerJoin=*/true,/*SemiJoin=*/true,/*AntiSemiJoin=*/true,/*OuterJoin=*/true,/*FullOuterJoin=*/false},
      /* OuterJoin    =  */{/*None=*/false,/*CP=*/true,/*InnerJoin=*/true,/*SemiJoin=*/true,/*AntiSemiJoin=*/true,/*OuterJoin=*/true,/*FullOuterJoin=*/false},
      /* FullOuterJoin=  */{/*None=*/false,/*CP=*/false,/*InnerJoin=*/false,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false}
   };
   bool r_asscom[OpType::LAST][OpType::LAST] = {
      /* None =  */{},
      /* CP           =  */{/*None=*/false,/*CP=*/true,/*InnerJoin=*/true,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false},
      /* InnerJoin    =  */{/*None=*/false,/*CP=*/true,/*InnerJoin=*/true,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false},
      /* SemiJoin     =  */{/*None=*/false,/*CP=*/false,/*InnerJoin=*/false,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false},
      /* AntiSemiJoin =  */{/*None=*/false,/*CP=*/false,/*InnerJoin=*/false,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false},
      /* OuterJoin    =  */{/*None=*/false,/*CP=*/false,/*InnerJoin=*/false,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false},
      /* FullOuterJoin=  */{/*None=*/false,/*CP=*/false,/*InnerJoin=*/false,/*SemiJoin=*/false,/*AntiSemiJoin=*/false,/*OuterJoin=*/false,/*FullOuterJoin=*/false}
   };
   // @formatter:on
   // clang-format on
   static OpType getOpType(Operator op) {
      return ::llvm::TypeSwitch<mlir::Operation*, OpType>(op.getOperation())
         .Case<mlir::relalg::CrossProductOp>([&](mlir::Operation* op) { return CP; })
         .Case<mlir::relalg::InnerJoinOp>([&](mlir::Operation* op) { return InnerJoin; })
         .Case<mlir::relalg::SemiJoinOp>([&](mlir::Operation* op) { return SemiJoin; })
         .Case<mlir::relalg::AntiSemiJoinOp>([&](mlir::Operation* op) { return AntiSemiJoin; })
         .Case<mlir::relalg::SingleJoinOp>([&](mlir::Operation* op) { return OuterJoin; })
         .Case<mlir::relalg::MarkJoinOp>([&](mlir::Operation* op) { return SemiJoin; }) //todo is this correct?
         .Case<mlir::relalg::OuterJoinOp>([&](mlir::relalg::OuterJoinOp op) {
            return op.join_direction() == mlir::relalg::JoinDirection::full ? FullOuterJoin : OuterJoin;
         })

         .Default([&](auto x) {
            return None;
         });
   }

   static bool is(const bool (&table)[OpType::LAST][OpType::LAST], Operator a, Operator b) {
      return table[getOpType(a)][getOpType(b)];
   }
   static std::pair<Operator, Operator> normalizeChildren(Operator op) {
      size_t left = 0;
      size_t right = 1;
      if (op->hasAttr("join_direction")) {
         mlir::relalg::JoinDirection joinDirection = mlir::relalg::symbolizeJoinDirection(
                                                        op->getAttr("join_direction").dyn_cast_or_null<mlir::IntegerAttr>().getInt())
                                                        .getValue();
         if (joinDirection == mlir::relalg::JoinDirection::right) {
            std::swap(left, right);
         }
      }
      return {op.getChildren()[left], op.getChildren()[right]};
   }
   static bool intersects(const attribute_set& a, const attribute_set& b) {
      return llvm::any_of(a, [&](auto x) { return b.contains(x); });
   }
   node_set calcTES(Operator b, NodeResolver& resolver) {
      if (TESs.count(b.getOperation())) {
         return TESs[b.getOperation()];
      } else {
         node_set TES = calcSES(b, resolver);
         auto children = b.getChildren();
         if (children.size() == 2) {
            auto [b_left, b_right] = normalizeChildren(b);
            for (auto a : b_left.getAllSubOperators()) {
               if (a.getChildren().size() == 2) {
                  auto [a_left, a_right] = normalizeChildren(a);
                  if (!is(assoc, a, b)) {
                     TES |= calcT(a_left, resolver);
                  }
                  if (!is(l_asscom, a, b)) {
                     TES |= calcT(a_right, resolver);
                  }
               } else {
                  if (mlir::isa<mlir::relalg::AggregationOp>(a.getOperation())) {
                     TES |= calcT(a, resolver);
                  }
               }
            }
            for (auto a : b_right.getAllSubOperators()) {
               if (a.getChildren().size() == 2) {
                  auto [a_left, a_right] = normalizeChildren(a);
                  if (!is(assoc, b, a)) {
                     TES |= calcT(a_right, resolver);
                  }
                  if (!is(r_asscom, b, a)) {
                     TES |= calcT(a_left, resolver);
                  }
               } else {
                  if (mlir::isa<mlir::relalg::AggregationOp>(a.getOperation())) {
                     TES |= calcT(a, resolver);
                  }
               }
            }

         } else if (children.size() == 1) {
            auto only_child = children[0];
            if (mlir::isa<mlir::relalg::AggregationOp>(b.getOperation())) {
               TES |= calcT(only_child, resolver);
            }
            if (auto renameop = mlir::dyn_cast_or_null<mlir::relalg::RenamingOp>(b.getOperation())) {
               for (auto a : only_child.getAllSubOperators()) {
                  if (intersects(a.getUsedAttributes(), renameop.getUsedAttributes()) || intersects(a.getCreatedAttributes(), renameop.getCreatedAttributes())) {
                     TES |= calcT(only_child, resolver);
                  }
               }
            }
         }
         TESs[b.getOperation()] = TES;
         return TES;
      }
   }

   NodeResolver populateQueryGraph(Operator op) {
      auto children = op.getChildren();
      auto used = op.getUsedAttributes();
      auto created = op.getCreatedAttributes();
      NodeResolver resolver(qg);
      if (already_optimized.count(op.getOperation())) {
         size_t new_node = addNode(op);
         for (auto attr : op.getAvailableAttributes()) {
            resolver.add(attr, new_node);
         }
         return resolver;
      }
      for (auto child : children) {
         resolver.merge(populateQueryGraph(child));
      }
      if (mlir::isa<mlir::relalg::CrossProductOp>(op.getOperation())) {
         //do not construct crossproducts in the querygraph
      } else if (mlir::isa<Join>(op.getOperation()) && !mlir::isa<mlir::relalg::InnerJoinOp>(op.getOperation())) {
         //add join edges into the query graph
         node_set TES = calcTES(op, resolver);
         node_set left_TES = calcT(children[0], resolver) & TES;
         node_set right_TES = calcT(children[1], resolver) & TES;
         qg.addEdge(expand(left_TES), expand(right_TES), empty_node, op, mlir::relalg::QueryGraph::EdgeType::REAL);
         if (!created.empty()) {
            size_t new_node = addNode(op);
            for (auto attr : op.getCreatedAttributes()) {
               resolver.add(attr, new_node);
            }
            qg.nodes[new_node].dependencies = expand(TES);
            qg.addEdge(expand(TES), node_set::single(num_nodes, new_node), empty_node, op, mlir::relalg::QueryGraph::EdgeType::IGNORE);
         }
      } else if (!created.empty()) {
         //add node for operators that create attributes
         size_t new_node = addNode(op);
         for (auto attr : op.getCreatedAttributes()) {
            resolver.add(attr, new_node);
         }
         if (children.size() == 1) {
            //if operator has one child e.g. aggregation/renaming/map
            // -> create "implicit" hyperedge
            node_set TES = calcTES(op, resolver);
            qg.nodes[new_node].dependencies = expand(TES);
            qg.addEdge(expand(TES), node_set::single(num_nodes, new_node), empty_node, op, mlir::relalg::QueryGraph::EdgeType::IMPLICIT);
         }
      } else if (mlir::isa<mlir::relalg::SelectionOp>(op.getOperation()) || mlir::isa<mlir::relalg::InnerJoinOp>(op.getOperation())) {
         node_set SES = calcSES(op, resolver);
         if (SES.count() == 1) {
            //if selection is only based on one node -> add selection to node
            auto node_id = SES.find_first();
            qg.nodes[node_id].additional_predicates.push_back(op);
         } else {
            auto first = SES.find_first();
            llvm::EquivalenceClasses<size_t> cannot_be_seperated;
            for (auto pos : SES) {
               cannot_be_seperated.insert(pos);
               for (auto dep : qg.nodes[pos].dependencies) {
                  cannot_be_seperated.unionSets(pos, dep);
               }
            }

            for (Operator subop : op.getAllSubOperators()) {
               if (subop != op) {
                  auto subop_TES = calcTES(subop, resolver);
                  if (SES.intersects(subop_TES) && !canPushSelection(SES, subop, resolver)) {
                     auto representant = (subop_TES & SES).find_first();
                     for (auto pos : (subop_TES & SES)) {
                        cannot_be_seperated.unionSets(representant, pos);
                     }
                  }
               }
            }
            if (cannot_be_seperated.getNumClasses() == 1) {
               qg.addEdge(getNodeSetFromClass(cannot_be_seperated, first, num_nodes), empty_node, empty_node, op, mlir::relalg::QueryGraph::EdgeType::REAL);
            } else {
               node_set decisions = empty_node;
               for (auto& a : cannot_be_seperated) {
                  if (a.isLeader()) {
                     decisions.set(a.getData());
                  }
               }
               decisions.iterateSubsets([&](node_set left) {
                  node_set right = decisions & ~left;
                  if (left < right) {
                     left = getNodeSetFromClasses(cannot_be_seperated, left);
                     right = getNodeSetFromClasses(cannot_be_seperated, right);

                     qg.addEdge(left, right, empty_node, op, mlir::relalg::QueryGraph::EdgeType::REAL);
                  }
               });
            }
         }
      } else {
         assert(false && " should not happen");
      }
      return resolver;
   }

   node_set calcSES(Operator op, NodeResolver& resolver) const {
      node_set res = node_set(num_nodes);
      for (auto attr : op.getUsedAttributes()) {
         res.set(resolver.resolve(attr));
      }
      return res;
   }

   std::unordered_map<mlir::Operation*, node_set> Ts;
   std::unordered_map<mlir::Operation*, node_set> TESs;
   std::unordered_map<mlir::Operation*, size_t> node_for_op;

   node_set calcT(Operator op, NodeResolver& resolver) {
      if (Ts.count(op.getOperation())) {
         return Ts[op.getOperation()];
      } else {
         node_set init = node_set(num_nodes);
         if (node_for_op.count(op.getOperation())) {
            init.set(node_for_op[op.getOperation()]);
         }
         if (!already_optimized.count(op.getOperation())) {
            for (auto child : op.getChildren()) {
               init |= calcT(child, resolver);
            }
         }
         Ts[op.getOperation()] = init;
         return init;
      }
   }
   bool canPushSelection(const node_set& SES, Operator curr, NodeResolver& resolver) {
      if (curr.getChildren().size() == 1) {
         return true;
      }
      auto TES = calcTES(curr, resolver);
      auto [b_left, b_right] = normalizeChildren(curr);
      node_set left_TES = calcT(b_left, resolver) & TES;
      node_set right_TES = calcT(b_right, resolver) & TES;
      if (left_TES.intersects(SES) && right_TES.intersects(SES)) {
         return false;
      }
      switch (getOpType(curr)) {
         case SemiJoin:
         case AntiSemiJoin:
         case OuterJoin:
            return !right_TES.intersects(SES);
         case FullOuterJoin: return false;
         default:
            return true;
      }
   }
   void ensureConnected() {
      llvm::EquivalenceClasses<size_t> already_connected;
      for (size_t i = 0; i < qg.getNodes().size(); i++) {
         already_connected.insert(i);
      }

      std::list<size_t> edges_to_process;
      for (size_t i = 0; i < qg.getEdges().size(); i++) {
         if (qg.getEdges()[i].left.any() && qg.getEdges()[i].right.any()) {
            edges_to_process.push_back(i);
         }
      }
      for (size_t i = 0; i < qg.getEdges().size(); i++) {
         std::list<size_t> new_list;
         for (auto edgeid : edges_to_process) {
            auto& edge = qg.getEdges()[edgeid];
            if (isConnected(already_connected, edge.left) && isConnected(already_connected, edge.right)) {
               already_connected.unionSets(edge.left.find_first(), edge.right.find_first());
            } else {
               new_list.push_back(edgeid);
            }
         }

         std::swap(edges_to_process, new_list);
      }
      for (auto& a : already_connected) {
         if (a.isLeader()) {
            for (auto& b : already_connected) {
               if (b.isLeader()) {
                  if (a.getData() != b.getData()) {
                     //std::cout << a.getData() << " vs " << b.getData() << "\n";
                     node_set left = getNodeSetFromClass(already_connected, a.getData(), num_nodes);
                     if (expand(left) != left) {
                        continue;
                     }
                     node_set right = getNodeSetFromClass(already_connected, b.getData(), num_nodes);
                     if (expand(right) != right) {
                        continue;
                     }
                     //std::cout << "would lead to edge (" << left << "," << right << ")\n";
                     bool connecting_edge_exists = false;
                     for (auto& edge : qg.getEdges()) {
                        if ((left.intersects(edge.left) && right.intersects(edge.right)) || (left.intersects(edge.right) && right.intersects(edge.left))) {
                           if (edge.op && !mlir::isa<mlir::relalg::SelectionOp>(edge.op.getOperation()) && !mlir::isa<mlir::relalg::InnerJoinOp>(edge.op.getOperation())) {
                              //std::cout << "but prohibited by edge (" << edge.left << "," << edge.right << ")\n";
                              connecting_edge_exists = true;
                           }
                           break;
                        }
                        if (left == edge.left || right == edge.right || left == edge.right || right == edge.left) {
                           if (edge.op && !mlir::isa<mlir::relalg::SelectionOp>(edge.op.getOperation()) && !mlir::isa<mlir::relalg::InnerJoinOp>(edge.op.getOperation())) {
                              //std::cout << "but prohibited by edge (" << edge.left << "," << edge.right << ")\n";
                              connecting_edge_exists = true;
                           }
                           break;
                        }
                     }
                     if (!connecting_edge_exists) {
                        qg.addEdge(left, right, node_set(num_nodes), Operator(), QueryGraph::EdgeType::REAL);
                     }
                  }
               }
            }
         }
      }
   }
   static node_set getNodeSetFromClass(llvm::EquivalenceClasses<size_t>& classes, size_t val, size_t num_nodes) {
      node_set res(num_nodes);
      auto eqclass = classes.findLeader(val);
      for (auto me = classes.member_end(); eqclass != me; ++eqclass) {
         res.set(*eqclass);
      }
      return res;
   }
   static node_set getNodeSetFromClasses(llvm::EquivalenceClasses<size_t>& classes, const node_set& S) {
      node_set res(S.size());
      for (auto pos : S) {
         auto eqclass = classes.findLeader(pos);
         for (auto me = classes.member_end(); eqclass != me; ++eqclass) {
            res.set(*eqclass);
         }
      }
      return res;
   }
   static bool isConnected(llvm::EquivalenceClasses<size_t>& connections, node_set& S) {
      assert(S.any());
      size_t first_class = S.size();
      bool connected = true;
      for (auto pos : S) {
         if (first_class == S.size()) {
            first_class = connections.getLeaderValue(pos);
         } else {
            if (first_class != connections.getLeaderValue(pos)) {
               connected = false;
            }
         }
      }
      return connected;
   }
   node_set expand(node_set S) {
      qg.iterateNodes(S, [&](QueryGraph::Node& n) {
         if (n.dependencies.valid()) {
            S |= n.dependencies;
         }
      });
      return S;
   }

   public:
   QueryGraphBuilder(Operator root, std::unordered_set<mlir::Operation*>& already_optimized) : root(root), already_optimized(already_optimized), num_nodes(countCreatingOperators(root)), qg(num_nodes, already_optimized), empty_node(num_nodes) {}
   void generate() {
      populateQueryGraph(root);
      ensureConnected();
   }
   QueryGraph& getQueryGraph() {
      return qg;
   }
};
}
#endif //DB_DIALECTS_QUERYGRAPHBUILDER_H