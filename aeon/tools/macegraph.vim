" Vim syntax file
" Language:	Mace graph file
" Maintainer:	Chip Killian <ckillian@cs.ucsd.edu>
" Last Change:	2006 July 11


"syn region bracket transparent start="\[" end="\]" contained oneline contains=Keyword,Todo,Type

syn keyword Keyword APP
syn keyword Special SCH
syn keyword Comment NET
"syn keyword Todo WARNING
syn keyword Error ERROR
syn keyword Todo FAILURE
syn keyword Error Assert
"syn keyword Keyword INFO

"syn match time "[0-9][0-9]*\.[0-9][0-9]*" contained contains=NONE
"syn match ip "[0-9][0-9]*.[0-9][0-9]*.[0-9][0-9]*.[0-9][0-9]*:[0-9][0-9]*" contained contains=NONE
"syn match macekey_ip "IPV4/[a-zA-Z0-9][a-zA-Z0-9.]*([a-zA-Z0-9][a-zA-Z0-9.]*):[0-9][0-9]*" contained contains=NONE
"syn match macekey_sha160 "SHA160/[a-zA-Z0-9][a-zA-Z0-9]*" contained contains=NONE
"syn match macekey_sha32 "SHA32/[a-zA-Z0-9][a-zA-Z0-9]*" contained contains=NONE
"syn match macekey_none "NONE/" contained contains=NONE
"syn match Identifier "^.\{-} \[.\{-}\]" contains=ip,macekey_ip,macekey_none,macekey_sha32,macekey_sha160,time,Type,Keyword,Todo,bracket,Error contained

syn match message_line_1 "^.*<----.*$"
syn match message_line_2 "^.*---->.*$"
syn match message_line_high_1 "^.*<- - -.*$"
syn match message_line_high_2 "^.*< - - .*$"
syn match message_line_high_3 "^.*- - ->.*$"
syn match message_line_high_4 "^.* - - >.*$"
syn match step_line_app "^.*APP.*$" contains=Keyword,Todo,Error,Special,Comment
syn match step_line_sch "^.*SCH.*$" contains=Keyword,Todo,Error,Special,Comment
syn match step_line_net "^.*NET.*$" contains=Keyword,Todo,Error,Special,Comment

hi link message_line_1 Constant
hi link message_line_2 Constant
hi link message_line_high_1 Type
hi link message_line_high_2 Type
hi link message_line_high_3 Type
hi link message_line_high_4 Type

hi link step_line_app Underlined 
hi link step_line_sch Underlined 
hi link step_line_net Underlined 

"hi link time Constant
"hi link ip Type
"hi link macekey_ip Type
"hi link macekey_sha160 Type
"hi link macekey_sha32 Type
"hi link macekey_none Type

"syn match line transparent "^.\{-}$" contains=Identifier,Type,Todo,Keyword,Error,ip,macekey_ip,macekey_none,macekey_sha32,macekey_sha160

