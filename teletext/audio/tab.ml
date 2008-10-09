let nums' =
  Array.of_list ((List.map
    (fun n -> 1. /. (3. *. 10. ** ((float_of_int n) /. 20.)))
    [0; 2; 4; 6; 8; 10; 12; 14; 16; 18; 20; 22; 24; 26; 28])
  @ [0.0])

(* These are the log volume levels used by BeebEm/SDL.  No idea how they were
   derived.  *)
let nums =
  Array.of_list (List.map
    (fun n -> (float_of_int n) /. (3. *. 128.))
    [120; 102; 87; 74; 63; 54; 46; 39; 33; 28; 24; 20; 17; 14; 11; 0])

let tab () =
  let voltab = Array.create 256 (0, 0, 0)
  and error = Array.create 256 0.0 in
  for i = 0 to 255 do
    let target = (float_of_int i) /. 255. in
    let closest = ref infinity
    and at, bt, ct = ref 16, ref 16, ref 16 in
    for a = 0 to 15 do
      for b = 0 to 15 do
        for c = 0 to 15 do
	  let av = nums.(a)
	  and bv = nums.(b)
	  and cv = nums.(c) in
	  let sum = av +. bv +. cv in
	  let distance = abs_float (sum -. target) in
	  if distance < !closest then begin
	    closest := distance;
	    at := a;
	    bt := b;
	    ct := c
	  end
	done
      done
    done;
    let best = nums.(!at) +. nums.(!bt) +. nums.(!ct) in
    voltab.(i) <- !at, !bt, !ct;
    error.(i) <- target -. best
    (*Printf.printf "%d, %f, %f, %f, %f, %f\n" i nums.(!at) nums.(!bt)
                    nums.(!ct) best error.(i) *)
  done;
  voltab, error

(* Convert block of 16-bit (unsigned) samples into 8-bit samples, where each
   sample value has a known quantisation error given by "quant". Perform 1-tap
   noise shaping to attempt to improve result.  *)

let convert_chunk buf outbuf quant start_err =
  (* Return sample in range 0.0 - 1.0.  *)
  let get_sample n =
    let samp = (Char.code buf.[n * 2]) + 256 * (Char.code buf.[n * 2 + 1]) in
    (float_of_int samp) /. 65535.0 in
  let clamp v = if v < 0 then 0 else if v > 255 then 255 else v in
  let prev_error = ref start_err in
  for i = 0 to (String.length outbuf) - 1 do
    let in_sample = (get_sample i) +. !prev_error in
    let index = int_of_float (in_sample *. 256.0) in
    let clamped_idx = clamp index in
    let err_for_sample = quant.(clamped_idx) in
    (* prev_error := err_for_sample; *)
    prev_error := 0.0;
    outbuf.[i] <- Char.chr clamped_idx
  done;
  !prev_error

let convert_file infile outfile quant =
  let inh = open_in_bin infile
  and outh = open_out_bin outfile in
  let inlen = in_channel_length inh
  and inbuf = String.create 8192
  and outbuf = String.make 4096 '\000' in
  let rec do_conversion bytes_remaining prev_err =
    if bytes_remaining > 0 then begin
      let bytes_to_do =
	if bytes_remaining < 8192 then bytes_remaining else 8192 in
      really_input inh inbuf 0 bytes_to_do;
      let next_err = convert_chunk inbuf outbuf quant prev_err in
      output outh outbuf 0 4096;
      do_conversion (bytes_remaining - bytes_to_do) next_err
    end in
  ignore (do_conversion inlen 0.0)

let write_volumes voltab =
  let outbuf = String.make 768 '\000' in
  for chan = 1 to 3 do
    for vol = 0 to 255 do
      let chanbits =
        match chan, voltab.(vol) with
          1, (a, _, _) -> 0b11010000 lor a
	| 2, (_, b, _) -> 0b10110000 lor b
	| 3, (_, _, c) -> 0b10010000 lor c
	| _ -> failwith "An unexpected thing" in
      outbuf.[(chan - 1) * 256 + vol] <- Char.chr chanbits
    done
  done;
  let ofile = open_out_bin "voltab" in
  output_string ofile outbuf;
  close_out ofile

let _ =
  let voltab, error = tab () in
  if (Array.length Sys.argv) != 3 then
    failwith (Printf.sprintf "Usage: %s <infile> <outfile>" Sys.argv.(0));
  let infile = Sys.argv.(1)
  and outfile = Sys.argv.(2) in
  convert_file infile outfile error;
  write_volumes voltab
