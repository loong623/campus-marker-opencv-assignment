"""Clang 原始词法 token 比较；额外保留预处理指令逻辑行及函数式宏邻接语义。"""
import hashlib,json,re,sys
from pathlib import Path
sys.path.insert(0,'/tmp/fix2-followup-development-tools')
from clang import cindex
OUT=Path(__file__).resolve().parent
INDEX=cindex.Index.create()
def signature(path,data):
 text=data.decode('utf-8');name=str(path.resolve())
 # 不解析依赖/AST；明确对整个物理文件范围取 Clang 原始词法 token。
 # 缺头文件诊断不影响该文件词法扫描；本项目另有两种真实编译/测试。
 tu=INDEX.parse(name,args=['-x','c++','-std=c++17','-nostdinc','-nostdinc++'],unsaved_files=[(name,text)],options=cindex.TranslationUnit.PARSE_INCOMPLETE)
 file=tu.get_file(name)
 extent=cindex.SourceRange.from_locations(cindex.SourceLocation.from_offset(tu,file,0),cindex.SourceLocation.from_offset(tu,file,len(data)))
 tokens=[token for token in tu.get_tokens(extent=extent) if token.kind!=cindex.TokenKind.COMMENT]
 spelling=[(token.kind.name,token.spelling) for token in tokens]
 first={}
 for token in tokens:first.setdefault(token.location.line,token)
 lines=data.splitlines(keepends=True);directives=[];i=0
 while i<len(lines):
  token=first.get(i+1)
  if token is None or token.spelling!='#':i+=1;continue
  start=i;end=i
  while end<len(lines)-1 and lines[end].rstrip(b'\r\n').endswith(b'\\'):end+=1
  group=[(t.kind.name,t.spelling) for t in tokens if start+1<=t.location.line<=end+1]
  logical=b''.join(lines[start:end+1]);logical=re.sub(rb'\\\r?\n',b'',logical)
  define=re.match(rb'\s*#\s*define\s+([A-Za-z_]\w*)(.*)',logical,re.S)
  function_like=bool(define and define.group(2).startswith(b'('))
  directives.append((group,function_like));i=end+1
 encoded=json.dumps({'tokens':spelling,'directives':directives},ensure_ascii=False,separators=(',',':')).encode()
 return {'sha256':hashlib.sha256(encoded).hexdigest(),'token_count':len(tokens),'directive_count':len(directives)},spelling,directives

def main():
 example=b'#include "absent.hpp"\n#define F(x) x + 1\n#define V (2)\nconst char* s=R"tag(a b\n#fake)tag";\nint f(){return F(V);}\n'
 fake=OUT/'lexical-selftest.cpp'
 original=signature(fake,example)
 assert original[0]['token_count']>20 and original[0]['directive_count']==3
 whitespace=example.replace(b'int f(){return',b'int   f()\n{\nreturn').replace(b'+ 1',b'+ /*note*/ 1')
 assert signature(fake,whitespace)[0]['sha256']==original[0]['sha256']
 for mutant in [example.replace(b'a b',b'ab'),example.replace(b'F(x)',b'F (x)'),example.replace(b'+ 1',b'- 1'),example.replace(b'\n#define V',b' #define V')]:
  assert signature(fake,mutant)[0]['sha256']!=original[0]['sha256']
 results=[]
 for source in (OUT/'files.txt').read_text().splitlines():
  path=Path(source);relative=path.relative_to('src/tushenghao');before=OUT/'before'/relative
  a=signature(path,before.read_bytes());b=signature(path,path.read_bytes())
  if a[1:]!=b[1:]:
   for k,(old,new) in enumerate(zip(a[1],b[1])):
    if old!=new:print('TOKEN DIFF',source,k,old,new);break
   raise AssertionError('format changed token/directive semantics: '+source)
  results.append({'file':source,'before_file_sha256':hashlib.sha256(before.read_bytes()).hexdigest(),'after_file_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'same_cpp_tokens':True,'same_preprocessor_logical_structure':True,**b[0]})
 report={'tool':'libclang 18.1.1 raw lexical tokens, whole-file SourceRange','exclusions':'只忽略 COMMENT token 和普通空白；保留字符串/字符/raw 字面量、运算符、标识符、逻辑预处理行和函数式宏邻接','selftests_pass':True,'files':results,'checked':len(results),'pass':True}
 (OUT/'token-check.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');print(json.dumps({'checked':len(results),'tokens_equal':True,'preprocessor_structure_equal':True,'selftests_pass':True}))
if __name__=='__main__':main()
