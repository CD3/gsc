let NERDTreeIgnore=['\.pdf']


let g:makeprg="ninja"
"let g:cmake_opts="-G Ninja"

let s:build_dir=expand('<sfile>:p:h')."/build"

map <F9> :AsyncRun ninja -C build<CR>

:CMake

call unite#custom#source('file_rec', 'ignore_pattern', '/build/.*' )
