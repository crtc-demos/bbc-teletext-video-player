(* ADFS writer.  *)

let byte0 n =
  Char.chr (n land 0xff)

let byte1 n =
  Char.chr ((n lsr 8) land 0xff)

let byte2 n =
  Char.chr ((n lsr 16) land 0xff)

let checksum buf =
  let sum = ref 255 in
  for i = 254 downto 0 do
    if !sum > 255 then
      sum := (succ !sum) land 0xff;
    sum := !sum + (Char.code buf.[i])
  done;
  Char.chr (!sum land 0xff)

type fsentry =
{
  fs_start : int;
  fs_length : int
}

let sector0 buf fsmap sectors =
  String.fill buf 0 0xf6 '\000';
  for i = 0 to (Array.length fsmap) - 1 do
    buf.[i * 3] <- byte0 fsmap.(i).fs_start;
    buf.[i * 3 + 1] <- byte1 fsmap.(i).fs_start;
    buf.[i * 3 + 2] <- byte2 fsmap.(i).fs_start
  done;
  String.fill buf 0xf6 (0xfb - 0xf6) '\000';
  buf.[0xfc] <- byte0 sectors;
  buf.[0xfd] <- byte1 sectors;
  buf.[0xfe] <- byte2 sectors;
  buf.[0xff] <- checksum buf

let sector1 buf fsmap diskid bootopt =
  String.fill buf 0 0xf6 '\000';
  for i = 0 to (Array.length fsmap) - 1 do
    buf.[i * 3] <- byte0 fsmap.(i).fs_length;
    buf.[i * 3 + 1] <- byte1 fsmap.(i).fs_length;
    buf.[i * 3 + 2] <- byte2 fsmap.(i).fs_length
  done;
  String.fill buf 0xf6 (0xfa - 0xf6) '\000';
  buf.[0xfb] <- byte0 diskid;
  buf.[0xfc] <- byte1 diskid;
  buf.[0xfd] <- byte0 bootopt;
  buf.[0xfe] <- byte0 (3 * (Array.length fsmap));
  buf.[0xff] <- checksum buf

let bcd num =
  let hi = num / 10
  and lo = num mod 10 in
  Char.chr (lo lor (hi lsl 4))

let getopt = function
    Some x -> x
  | None -> raise Not_found

(* This buf should be 0x500 bytes long.  *)

let dir_header buf seqno =
  buf.[0] <- bcd seqno;
  buf.[1] <- 'H';
  buf.[2] <- 'u';
  buf.[3] <- 'g';
  buf.[4] <- 'o'

let setb7 buf offset =
  let code = Char.code buf.[offset] in
  buf.[offset] <- Char.chr (code lor 128)

let i32b0 n =
  Char.chr (Int32.to_int (Int32.logand n 0xffl))

let i32b1 n =
  Char.chr (Int32.to_int (Int32.logand (Int32.shift_right_logical n 8) 0xffl))

let i32b2 n =
  Char.chr (Int32.to_int (Int32.logand (Int32.shift_right_logical n 16) 0xffl))

let i32b3 n =
  Char.chr (Int32.to_int (Int32.logand (Int32.shift_right_logical n 24) 0xffl))

type fobj =
{
  f_name : string;
  f_access : [`R | `W | `L | `D | `E] list;
  f_load : int32;
  f_exec : int32;
  f_length : int32;
  f_srcpath : string option;
  mutable f_startsec : int option;
  mutable f_seqno : int option
}

(* Dir entries start at dir + 5.  *)

let dir_entry buf offset obj =
  String.fill buf offset 10 '\013';
  String.blit obj.f_name 0 buf offset (String.length obj.f_name);
  if List.mem `R obj.f_access then setb7 buf (offset + 0);
  if List.mem `W obj.f_access then setb7 buf (offset + 1);
  if List.mem `L obj.f_access then setb7 buf (offset + 2);
  if List.mem `D obj.f_access then setb7 buf (offset + 3);
  if List.mem `E obj.f_access then setb7 buf (offset + 4);
  buf.[offset + 0x0a] <- i32b0 obj.f_load;
  buf.[offset + 0x0b] <- i32b1 obj.f_load;
  buf.[offset + 0x0c] <- i32b2 obj.f_load;
  buf.[offset + 0x0d] <- i32b3 obj.f_load;
  buf.[offset + 0x0e] <- i32b0 obj.f_exec;
  buf.[offset + 0x0f] <- i32b1 obj.f_exec;
  buf.[offset + 0x10] <- i32b2 obj.f_exec;
  buf.[offset + 0x11] <- i32b3 obj.f_exec;
  buf.[offset + 0x12] <- i32b0 obj.f_length;
  buf.[offset + 0x13] <- i32b1 obj.f_length;
  buf.[offset + 0x14] <- i32b2 obj.f_length;
  buf.[offset + 0x15] <- i32b3 obj.f_length;
  buf.[offset + 0x16] <- byte0 (getopt obj.f_startsec);
  buf.[offset + 0x17] <- byte1 (getopt obj.f_startsec);
  buf.[offset + 0x18] <- byte2 (getopt obj.f_startsec);
  buf.[offset + 0x19] <- bcd (getopt obj.f_seqno)

let dir_footer buf name title parentsec seqno =
  buf.[0x4cb] <- '\000';
  String.fill buf 0x4cc (0x4d5 - 0x4cc) '\013';
  String.blit name 0 buf 0x4cc (String.length name);
  buf.[0x4d6] <- byte0 parentsec;
  buf.[0x4d7] <- byte1 parentsec;
  buf.[0x4d8] <- byte2 parentsec;
  String.fill buf 0x4d9 (0x4eb - 0x4d9) '\013';
  String.blit title 0 buf 0x4d9 (String.length title);
  String.fill buf 0x4ec (0x4f9 - 0x4ec) '\000';
  buf.[0x4fa] <- bcd seqno;
  buf.[0x4fb] <- 'H';
  buf.[0x4fc] <- 'u';
  buf.[0x4fd] <- 'g';
  buf.[0x4fe] <- 'o';
  buf.[0x4ff] <- '\000'

let mkdir name len srcpath =
  {
    f_name = name;
    f_access = [`D; `L; `R];
    f_load = 0l;
    f_exec = 0l;
    f_length = Int32.of_int len;
    f_srcpath = srcpath;
    f_startsec = None;
    f_seqno = None
  }

type ftree =
    File of fobj
  | Dir of fobj * ftree list

(* Loads simple one-line ".inf" files with format:
   <filename> <load-addr> <exec-addr>.
   "filename" is ignored.  (Trying for approximate compatibility with other
   BBC disk-imaging programs.)
*)

let read_inf inf =
  let chan = open_in inf in
  try
    let line = input_line chan in
    let attrs = Scanf.sscanf line " %s %lx %lx"
      (fun _ load exec -> load, exec) in
    close_in chan;
    attrs
  with End_of_file ->
    failwith (Printf.sprintf "Can't read .inf file '%s'" inf)
  | Scanf.Scan_failure msg ->
    failwith (Printf.sprintf "Can't read .inf file '%s', %s" inf msg)

let sort_objects objlist =
  let objname = function
    File f -> String.lowercase f.f_name
  | Dir (d, _) -> String.lowercase d.f_name in
  List.sort 
    (fun a b -> let a' = objname a and b' = objname b in compare a' b')
    objlist

let gather_files dir =
  let list_of_files dir =
    Array.to_list (Sys.readdir dir) in
  let rec scanfiles dir = function
    [] -> []
  | f::fs ->
      if Sys.is_directory (Filename.concat dir f) then
	let newdir = Filename.concat dir f in
	let dirfiles = scanfiles newdir (list_of_files newdir) in
	let dirlen = List.length dirfiles in
        let item = Dir (mkdir f dirlen (Some newdir), sort_objects dirfiles) in
        item :: scanfiles dir fs
      else begin
        if Filename.check_suffix f ".inf" then begin
	  let unsuff = Filename.chop_suffix f ".inf" in
	  let qual = Filename.concat dir unsuff in
	  if Sys.file_exists qual then begin
	    let load, exec = read_inf (Filename.concat dir f) in
	    let item = File {
	      f_name = unsuff;
	      f_access = [`R; `W];
	      f_load = load;
	      f_exec = exec;
	      f_length = Int32.of_int ((Unix.stat qual).Unix.st_size);
	      f_srcpath = Some qual;
	      f_startsec = None;
	      f_seqno = None
	    } in
	    item :: scanfiles dir fs
	  end else
	    failwith ("No actual file for " ^ (Filename.concat dir f))
	end else
	  scanfiles dir fs
      end
  in let files = list_of_files dir in
  let rootfiles = scanfiles dir files in
  Dir (mkdir "$" (List.length rootfiles) None, sort_objects rootfiles)

let do_layout tree =
  let num_sectors size =
    Int32.to_int (Int32.shift_right_logical (Int32.add size 255l) 8) in
  let rec layout_dir sector seqno = function
    [] -> sector
  | obj::objs ->
      match obj with
        File fobj ->
          fobj.f_startsec <- Some sector;
	  (* ??? do objects have sequence number of parent dir?  *)
	  fobj.f_seqno <- Some seqno;
	  let obj_size = num_sectors fobj.f_length in
	  layout_dir (sector + obj_size) seqno objs
      | Dir (dobj, dcontents) ->
          dobj.f_startsec <- Some sector;
	  dobj.f_seqno <- Some seqno; (* ???.  *)
	  let sector' = layout_dir (sector + 5) seqno objs in
	  layout_dir sector' (succ seqno) dcontents
  in
    layout_dir 2 0 tree

(* Create a 1-entry free space map for the rest of the space in the image.  *)
let create_fsmap lastused totalsectors =
  [| { fs_start = lastused; fs_length = totalsectors - lastused } |]

type image_info =
{
  im_sectors : int;
  im_bootopt : int;
  im_diskid : int
}

let write_output ochan layout_tree fsmap iminfo =
  let secsize = 256 in
  let sec0 = String.create secsize
  and sec1 = String.create secsize in
  sector0 sec0 fsmap iminfo.im_sectors;
  sector1 sec1 fsmap iminfo.im_diskid iminfo.im_bootopt;
  seek_out ochan 0;
  output_string ochan sec0;
  output_string ochan sec1;
  let rec emit parentsec = function
    [] -> ()
  | File fobj :: fs ->
      seek_out ochan ((getopt fobj.f_startsec) * secsize);
      begin match fobj.f_srcpath with
        Some filename ->
          let ichan = open_in_bin (getopt fobj.f_srcpath) in
	  let ilen = in_channel_length ichan in
	  let idata = String.create ilen in
	  really_input ichan idata 0 ilen;
	  output_string ochan idata;
	  close_in ichan
      | None -> failwith (Printf.sprintf "No source file for '%s'" fobj.f_name)
      end;
      emit parentsec fs
  | Dir (dobj, dcontents) :: fs ->
      let dirbuf = String.create 0x500 in
      dir_header dirbuf (getopt dobj.f_seqno);
      ignore (List.fold_left
        (fun objnum node ->
	  let obj = match node with
	    File fobj -> fobj
	  | Dir (dobj, _) -> dobj in
	  dir_entry dirbuf (objnum * 0x1a + 5) obj;
	  succ objnum)
	0
	dcontents);
      dir_footer dirbuf dobj.f_name dobj.f_name parentsec (getopt dobj.f_seqno);
      seek_out ochan ((getopt dobj.f_startsec) * secsize);
      output_string ochan dirbuf;
      emit parentsec fs;
      emit (getopt dobj.f_startsec) dcontents;
  in
    emit 2 [layout_tree]

let _ =
  if (Array.length Sys.argv) != 3 then
    failwith (Printf.sprintf "Usage: %s <outfile> <dirname>" Sys.argv.(0));
  let layout = gather_files Sys.argv.(2) in
  (* 10 megabyte disk, ooh!  *)
  let disksize = 10 * 1024 * 1024 / 256 in
  let diskid = 999 in
  let sector_total = do_layout [layout] in
  let fsmap = create_fsmap sector_total disksize in
  let ochan = open_out_bin Sys.argv.(1) in
  write_output ochan layout fsmap
    { im_sectors = disksize; im_bootopt = 4; im_diskid = diskid };
  close_out ochan
