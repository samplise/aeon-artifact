" Vim syntax file
" Language:	Mace Logfiles
" Maintainer:	Chip Killian <ckillian@cs.ucsd.edu>
" Last Change:	2005 July 07


"syn region bracket transparent start="\[" end="\]" contained oneline contains=Keyword,Todo,Type

syn keyword Keyword REPLAY
syn keyword Debug DEBUG
syn keyword Todo WARNING
syn keyword Error ERROR
syn keyword Keyword INFO
syn keyword PreProc TRACE

syn match time "[0-9][0-9]*\.[0-9][0-9]*" contained contains=NONE
syn match timelong "^\[.\{-}\] " contained contains=NONE
syn match ip "[0-9][0-9]*.[0-9][0-9]*.[0-9][0-9]*.[0-9][0-9]*:[0-9][0-9]*" contained contains=NONE
syn match macekey_ip_host "IPV4/[a-zA-Z0-9][a-zA-Z0-9.]*([a-zA-Z0-9][a-zA-Z0-9.]*):[0-9][0-9]*" contained contains=NONE
syn match macekey_ip "IPV4/[a-zA-Z0-9][a-zA-Z0-9.]*:[0-9][0-9]*" contained contains=NONE
syn match macekey_sha160 "SHA160/[a-zA-Z0-9][a-zA-Z0-9]*" contained contains=NONE
syn match macekey_sha32 "SHA32/[a-zA-Z0-9][a-zA-Z0-9]*" contained contains=NONE
syn match macekey_string "STRING/\[.\{-}\]" contained contains=NONE
syn match macekey_none "NONE/" contained contains=NONE
syn match Identifier "^.\{-} \[.\{-}\]" contains=ip,macekey_ip,macekey_ip_host,macekey_none,macekey_sha32,macekey_sha160,macekey_string,time,timelong,Type,Keyword,Todo,bracket,PreProc,Debug,Error contained

hi link time Constant
hi link timelong Constant
hi link ip Type
hi link macekey_ip Type
hi link macekey_ip_host Type
hi link macekey_sha160 Type
hi link macekey_sha32 Type
hi link macekey_none Type
hi link macekey_string Type

syn match line transparent "^.\{-}$" contains=PreProc,Identifier,Type,Todo,Keyword,Error,Debug,ip,macekey_ip,macekey_ip_host,macekey_string,macekey_none,macekey_sha32,macekey_sha160

