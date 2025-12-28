#include <algorithm>
#include <filesystem>
#include <unordered_map>

#include "Driver.hpp"
#include "Error.hpp"
#include "FSHelper.hpp"
#include "Parser.hpp"
#include "Utils.hpp"

namespace fire {

NdModule* Parser::parse() {
  auto mod = ps_mod();
  merge_namespaces(mod->items);
  reorder_items(mod->items);
  return mod;
}

void Parser::ps_do_import(Token* import_token, std::string path) {
  if (!std::filesystem::exists(path)) {
    if (std::filesystem::exists(path + ".fire")) {
      path += ".fire";
      auto imported = source.import(path);
      if (imported->get_depth() >= 4) {
        throw err::parses::import_depth_limit_exceeded(*import_token,
                                                       imported->path);
      }
    } else {
      throw err::parses::cannot_open_file(*import_token, path);
    }
  } else if (std::filesystem::is_directory(path)) {
    source.import_directory(path);
  }
}

void Parser::ps_import() {
  auto import_token = cur;

  std::string path = expect_ident()->text;

  while (!is_end() && eat("::")) {
    path += "/";

    if (eat("*")) {
      todo;
    }

    path += expect_ident()->text;
  }

  expect(";");

  try {
    ps_do_import(
        import_token,
        std::filesystem::absolute(source.get_folder() + path).string());
  } catch (err::parses::cannot_open_file& e) {
    std::string fol = FileSystem::GetFolderOfFile(source.path);

    while (true) {
      if (fol == Driver::get_instance()->get_first_cwd()) {
        e.msg.pop_back();
        e.msg += ".fire'";
        throw e;
      }

      fol = FileSystem::GetFolderOfFile(fol);

      if (auto tmp = fol + "/" + path; FileSystem::IsFile(tmp + ".fire"))
        ps_do_import(import_token, tmp + ".fire");
      else if (FileSystem::IsDirectory(tmp))
        ps_do_import(import_token, tmp);

      return;
    }
    throw e;
  }
}

void Parser::merge_namespaces(std::vector<Node*>& items) {
  std::unordered_map<std::string, NdNamespace*> table;

  for (Node*& node : items) {
    if (!node->is(NodeKind::Namespace)) continue;

    auto ns = node->as<NdNamespace>();

    auto [it, inserted] = table.emplace(ns->name, ns);

    if (inserted) continue;

    auto original = it->second;

    original->items.insert(original->items.end(), ns->items.begin(),
                           ns->items.end());

    ns->items.clear();
    delete ns;

    node = nullptr;
  }

  items.erase(std::remove(items.begin(), items.end(), nullptr), items.end());

  for (Node* node : items)
    if (node->is(NodeKind::Namespace))
      merge_namespaces(node->as<NdNamespace>()->items);
}

void Parser::reorder_items(std::vector<Node*>& items) {
  std::vector<Node*> V;

  V.reserve(items.size());

  auto appender = [&items, &V](NodeKind K) {
    for (Node* x : items)
      if (x->is(K)) V.push_back(x);
  };

  appender(NodeKind::Namespace);
  appender(NodeKind::Enum);
  appender(NodeKind::Class);
  appender(NodeKind::Function);

  items = std::move(V);

  for (auto&& x : items) {
    if (x->is(NodeKind::Namespace))
      reorder_items(x->as<NdNamespace>()->items);
    else if (x->is(NodeKind::Module))
      reorder_items(x->as<NdModule>()->items);
  }
}

} // namespace fire