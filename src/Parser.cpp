#include <filesystem>

#include "Driver.hpp"
#include "Error.hpp"
#include "FSHelper.hpp"
#include "Parser.hpp"
#include "Utils.hpp"
#include "strconv.hpp"

namespace fire {

NdSymbol* Parser::ps_type_name() {
  Token& tok = *cur;

  //
  // parse syntax suger for tuple type
  // (T, U, ...) --> tuple<T, U, ...>
  if (eat("(")) {
    NdSymbol* tuple_type = new NdSymbol(tok);

    tuple_type->name.kind = TokenKind::Identifier;
    tuple_type->name.text = "tuple";
    tuple_type->te_args.push_back(ps_type_name());

    expect_comma();

    do {
      tuple_type->te_args.push_back(ps_type_name());
    } while (!is_end() && eat_comma());

    expect(")");

    return tuple_type;
  }

  if (eat("decltype")) {
    expect("(");

    NdDeclType* x = new NdDeclType(tok, ps_expr());

    expect(")");

    NdSymbol* s = new NdSymbol(tok);
    s->dec = x;

    return s;
  }

  return ps_symbol(true);
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
  bool flag = false;
__begin__:;
  flag = false;
  for (size_t i = 0; i < items.size();) {
    if (auto orig = items[i]->as<NdNamespace>();
        orig->is(NodeKind::Namespace)) {
      for (size_t j = i + 1; j < items.size(); j++) {
        if (auto dup = items[j]->as<NdNamespace>();
            dup->is(NodeKind::Namespace) && dup->name == orig->name) {
          for (auto x : dup->items)
            orig->items.push_back(x);
          delete dup;
          items.erase(items.begin() + j);
          flag = true;
          goto __merged;
        }
      }
      i++;
    __merged:;
    } else {
      i++;
    }
  }
  if (flag)
    goto __begin__;
  for (auto&& x : items) {
    if (x->is(NodeKind::Namespace))
      merge_namespaces(x->as<NdNamespace>()->items);
  }
}

void Parser::reorder_items(std::vector<Node*>& items) {
  // move all enums at first
  std::vector<Node*> _new;
  for (auto&& x : items)
    if (x->is(NodeKind::Let))
      _new.push_back(x);
  for (auto&& x : items)
    if (x->is(NodeKind::Namespace))
      _new.push_back(x);
  for (auto&& x : items)
    if (x->is(NodeKind::Enum))
      _new.push_back(x);
  for (auto&& x : items)
    if (x->is(NodeKind::Class))
      _new.push_back(x);
  for (auto&& x : items)
    if (x->is(NodeKind::Function))
      _new.push_back(x);
  items = std::move(_new);
  for (auto&& x : items) {
    if (x->is(NodeKind::Namespace))
      reorder_items(x->as<NdNamespace>()->items);
    else if (x->is(NodeKind::Module))
      reorder_items(x->as<NdModule>()->items);
  }
}

NdModule* Parser::parse() {
  auto mod = ps_mod();
  merge_namespaces(mod->items);
  reorder_items(mod->items);
  return mod;
}

} // namespace fire