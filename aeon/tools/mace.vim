" Install this file to ~/.vim/syntax/mace.vim.

runtime! syntax/cpp.vim
let b:current_syntax = "mace"

syn keyword     maceBlock      messages state_variables states auto_types typedefs service structured_logging constructor_parameters method_remappings transitions routines properties detect queries trace constants services time variant variants registration
syn keyword     maceBlock      safety liveness uses provides upcalls downcalls

syn keyword     maceAttribute __attribute contained

syn keyword     maceStorageClass   downcall upcall aspect scheduler sync async

syn keyword     maceType           MaceKey registration_uid_t NodeSet timer ServiceType MaceTime

syn match     maceInclude    "#minclude"

syn keyword     maceTraceConstant off manual low high medium med
syn keyword     maceTimeConstant curtime
syn keyword     maceKeyConstant ipv4 sha32 sha160 string_key

syn keyword     maceAttributeConstant contained no default yes
syn keyword     maceAttributeKeyword dump serialize state node comparable equals lessthan hashof private score reset mutable notComparable printNode pointer reset recur
syn keyword     maceNoHighlight maceAttributeConstant contained no default yes maceAttributeKeyword dump serialize node comparable equals lessthan hashof private score reset mutable notComparable printNode pointer reset recur

syn keyword     maceKeyword state dynamic

syn keyword     maceFunction upcallAll upcallAllVoid 
syn keyword     maceDebug macedbg ASSERT maceout maceLog maceerr macewarn maceWarn maceError ABORT ASSERTMSG EXPECT

syn match       macePropertyKeyword /\\for{[^}]*}{[^}]*}/
syn match       macePropertyKeyword /\\forall/
syn match       macePropertyKeyword /\\exists/

syn match       macePropertyKeyword /\\nodes/

syn match       macePropertyKeyword /\\in/
syn match       macePropertyKeyword /\\notin/

syn match       macePropertyKeyword /\\neq/
syn match       macePropertyKeyword /\\eq/
syn match       macePropertyKeyword /\\leq/
syn match       macePropertyKeyword /\\geq/

syn match       macePropertyKeyword /\\and/
syn match       macePropertyKeyword /\\or/
syn match       macePropertyKeyword /\\implies/
syn match       macePropertyKeyword /\\xor/
syn match       macePropertyKeyword /\\iff/

syn match       macePropertyKeyword /\\not/
syn match       macePropertyKeyword /\\subset/
syn match       macePropertyKeyword /\\propersubset/
syn match       macePropertyKeyword /\\null/

syn region maceAttributeMatch start=/__attribute[(][(]/ end=/[)][)][^)]/ contains=maceAttributeConstant,maceAttribute,maceAttributeKeyword

hi def link maceBlock Statement
hi def link maceStatement Statement
hi def link maceType Type
hi def link maceInclude Include
hi def link maceStorageClass StorageClass
hi def link maceTraceConstant Constant
hi def link maceTimeConstant Constant
hi def link maceKeyConstant Constant
hi def link macePropertyKeyword Keyword
hi def link maceKeyword Keyword
hi def link maceAttribute Keyword
hi def link maceAttributeConstant Constant
hi def link maceAttributeKeyword Function

hi def link maceDebug Debug

"syn keyword     cConstant       SUCCESS FAIL TRUE FALSE
