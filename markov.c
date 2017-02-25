/* markov.c */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <string.h>
#include <errno.h>
enum{
NPREF = 4, /* プレフィクスの語数 */
NHASH = 4093, /* 状態ハッシュテーブル配列のサイズ */
MAXGEN = 10000 /* 生成される単語数の上限 */
};
typedef struct State State;
typedef struct Suffix Suffix;
struct State{ /* プレフィクス+サフィックスリスト */
  char *pref[NPREF]; /* プレフィクスの単語 */ Suffix *suf; /* サフィックスリスト */
State *next; /* ハッシュテーブルの中の次の要素 */
};
struct Suffix{/* サフィックスリスト */
char *word; /* suffix */
Suffix *next; /* サフィックスリストの中の次の要素 */
};
State *statetab[NHASH]; /* 状態のハッシュテーブル */
static char *name = NULL; /* メッセージ用のプログラム名 */
char NONWORD[] = "\n"; /* 実際の単語としては絶対に出現しない */
char *progname(void)
{
  return name;
}
/* eprint: エラーメッセージを表示して終了 */ void eprintf(char *fmt, ...)
{
  va_list args;
  fflush(stdout);

  if(progname() != NULL)
    fprintf(stderr, "%s: ", progname());
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
if(fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':')
fprintf(stderr, "  %s", strerror(errno));
  fprintf(stderr, "\n");
exit(2); /* 実行の失敗を表す慣習的な値 */ }
/* estrdup: 文字列をコピー、エラー時には報告 */
char *estrdup(char *s)
{
char *t;
  t = (char *) malloc(strlen(s)+1);
  if(t == NULL)
    eprintf("estrdup(\"%.20s\") failed:", s);
  strcpy(t, s);
  return t;
}
/* emalloc: mallocを実行し、エラー時には報告 */
void *emalloc(size_t n)
{
void *p;
  p = malloc(n);
  if(p == NULL)
    eprintf("malloc of %u bytes failed:",n);
  return p;
}
const int MULTIPLIER = 31; /* hash() 用 */
/* hash: NPREF個の文字列からなる配列のハッシュ値を計算 */
unsigned int hash(char *s[NPREF])
{
  unsigned int h;
  unsigned char *p;
  int i;
  h = 0;
  for(i = 0; i < NPREF; i++)
    for(p = (unsigned char *) s[i]; *p != '\0'; p++)
      h = MULTIPLIER * h + *p;
  return h % NHASH;
}
/* lookup: プレフィクスを検索。指定されればそれを生成 */
/* 見つかるか生成したらポインタを、そうでなければ NULL を返す */
/* 生成作業は strdup しないので、あとで文字列が変化してはならない */
State* lookup(char *prefix[NPREF], int create)
{
  int i,h;
  State *sp;
  h = hash(prefix);
  for(sp = statetab[h]; sp != NULL; sp = sp->next) {
    for(i = 0; i < NPREF; i++)
      if(strcmp(prefix[i], sp->pref[i]) != 0)
break;
if(i == NPREF) /* 見つかった */
return sp;
  }
  if(create) {
    sp = (State *) emalloc(sizeof(State));
    for (i = 0; i < NPREF; i++)
      sp->pref[i] = prefix[i];
    sp->suf = NULL;
    sp->next = statetab[h];
    statetab[h] = sp;
}
return sp; }

/* addsuffix: 状態に追加。あとでサフィックスが変化してはならない */
void addsuffix(State *sp, char *suffix)
{
Suffix *suf;
  suf = (Suffix *) emalloc(sizeof(Suffix));
  suf->word = suffix;
  suf->next = sp->suf;
  sp->suf = suf;
}
/* add: 単語をサフィックスリストに追加し、プレフィクスを更新 */
void add(char *prefix[NPREF], char *suffix)
{
State *sp;
sp = lookup(prefix, 1); /* 見つからなければ生成 */
addsuffix(sp, suffix);
//printf("suffix:%s\n", suffix);
/* プレフィクス中の単語を前にずらす */
memmove(prefix, prefix+1, (NPREF-1)*sizeof(prefix[0]));
prefix[NPREF-1] = suffix;
}
/* build: 入力を読み、プレフィクステーブルを作成 */
void build(char *prefix[NPREF], FILE *f)
{
  char buf[100],fmt[10];
/* 書式文字列を作成。ただの %s だと buf がオーバーフローする可能性がある */
sprintf(fmt, "%%%ds", sizeof(buf)-1);
//printf("fmt:%s\n", fmt);
while (fscanf(f, fmt, buf) != EOF)
    add(prefix, estrdup(buf));
}
/* generate: 1行に1語ずつ出力を生成 */
void generate(int nwords)
{
State *sp;

Suffix *suf;
  char *prefix[NPREF], *w;
  int i, nmatch;
for (i = 0; i < NPREF; i++) /* 初期プレフィクスをリセット */
prefix[i] = NONWORD;
  for(i = 0; i < nwords; i++) {
    sp = lookup(prefix, 0);
    nmatch = 0;
    for(suf = sp->suf; suf != NULL; suf = suf->next)
if(rand() % ++nmatch == 0) /* 確率 = 1/nmatch */ w = suf->word;
    if(strcmp(w, NONWORD) == 0)
      break;
    printf("%s\n", w);
    memmove(prefix, prefix+1, (NPREF-1)*sizeof(prefix[0]));
    prefix[NPREF-1] = w;
} }
/* markov main: マルコフ連鎖によるランダムテキスト生成プログラム */
int main(void)
{
int i, nwords = MAXGEN;
char *prefix[NPREF]; /* 現在の入力プレフィクス */
for(i = 0; i < NPREF; i++) /* 初期プレフィクスをセットアップ */
prefix[i] = NONWORD;
  build(prefix, stdin);
  add(prefix, NONWORD);
  generate(nwords);
  return 0;
}
