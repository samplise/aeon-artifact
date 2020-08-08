" Vim syntax file
" Language:	mh -- mace service class header file
" Maintainer:	Charles Killian <ckillian@cs.ucsd.edu>
" Last Change:	2005 Sept 6
" Copied from cpp

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Read the C syntax to start with
if version < 600
  so <sfile>:p:h/cpp.vim
else
  runtime! syntax/cpp.vim
  unlet b:current_syntax
endif

" mh extentions
syn keyword cppStructure        handlers

let b:current_syntax = "mh"

" vim: ts=8
