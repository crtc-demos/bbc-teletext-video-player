(* Recode algorithm:

   We have 3 channels with logarithmic volume scales.  They are combined
   linearly (I assume!) to make the final sample.
   
   The initial plan was to find the best combination of channel volumes for
   each of 256 overall levels, to give (almost) 8-bit resolution for sample
   playback. Unfortunately we can't set the volumes of three channels
   *instantaneously*, and spikes introduced during the volume-change operation
   serve to drown the desired sample with noise.
   
   So, here's the alternative plan. We'll try to process the sample into a
   sequence of commands, which can either:
   
   (a) change a particular channel to a particular level.
   (b) change *all* channels to a particular level.
       - in an order such that intermediate volume levels are *inbetween* the
         current sample and the desired one.
*)

let nums =
  Array.of_list ((List.map
    (fun n -> 1. /. (3. *. 10. ** ((float_of_int n) /. 20.)))
    [0; 2; 4; 6; 8; 10; 12; 14; 16; 18; 20; 22; 24; 26; 28])
  @ [0.0])

(* These are the log volume levels used by BeebEm/SDL.  No idea how they were
   derived.  *)
let nums' =
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
    if true then
      prev_error := err_for_sample
    else
      prev_error := 0.0;
    outbuf.[i] <- Char.chr clamped_idx
  done;
  !prev_error

type sndstate =
{
  chan1 : int;
  chan2 : int;
  chan3 : int
}

type cmd =
    Chan1 of int
  | Chan2 of int
  | Chan3 of int
  | Achan of int
  | Nocmd

let sconv inbuf outbuf state =
  let lookahead = 10 in
  let memos = Hashtbl.create 4096 in
  let get_sample n =
    let samp = (Char.code inbuf.[n * 2])
	       + 256 * (Char.code inbuf.[n * 2 + 1]) in
    (float_of_int samp) /. 65535.0 in
  let level st =
    nums.(st.chan1) +. nums.(st.chan2) +. nums.(st.chan3) in
  let minimise state samp modfn =
    let best_val = ref (-1)
    and lowest_err = ref infinity
    and winning_state = ref state in
    for i = 0 to 15 do
      let modified_state = modfn state i in
      let thisval = abs_float ((level modified_state) -. samp) in
      if thisval < !lowest_err then begin
        best_val := i;
	lowest_err := thisval;
	winning_state := modified_state
      end
    done;
    !best_val, !lowest_err, !winning_state in
  let rec search st n lah =
    if lah = 0 || (2 * n) >= (String.length inbuf) then
      0.0, st, Nocmd
    else begin
      let this_samp = get_sample n in
      (* Costs if channel 1 is altered.  *)
      let chan1_lev, chan1_err, chan1_st =
	minimise st this_samp (fun st' n -> { st' with chan1 = n }) in
      let chan1_lookahead_err, _, _ =
        memoized_search chan1_st (succ n) (pred lah) in
      let chan1_err_tot = chan1_err +. chan1_lookahead_err in
      (* Costs if channel 2 is altered.  *)
      let chan2_lev, chan2_err, chan2_st =
	minimise st this_samp (fun st' n -> { st' with chan2 = n }) in
      let chan2_lookahead_err, _, _ =
        memoized_search chan2_st (succ n) (pred lah) in
      let chan2_err_tot = chan2_err +. chan2_lookahead_err in
      (* Costs if channel 3 is altered.  *)
      let chan3_lev, chan3_err, chan3_st =
	minimise st this_samp (fun st' n -> { st' with chan3 = n }) in
      let chan3_lookahead_err, _, _ =
        memoized_search chan3_st (succ n) (pred lah) in
      let chan3_err_tot = chan3_err +. chan3_lookahead_err in
      (* Costs if all channels are altered.  *)
      let allchan_lev, allchan_err, allchan_st =
	minimise st this_samp
          (fun st' n -> { chan1 = n; chan2 = n; chan3 = n }) in
      let allchan_lookahead_err, _, _ =
        memoized_search allchan_st (succ n) (pred lah) in
      let allchan_err_tot = allchan_err +. allchan_lookahead_err in
      (* And the winner is...  *)
      if chan1_err_tot <= chan2_err_tot
	 && chan1_err_tot <= chan3_err_tot
	 && chan1_err_tot <= allchan_err_tot then begin
	chan1_err_tot, chan1_st, (Chan1 chan1_lev)
      end else if chan2_err_tot <= chan1_err_tot
                  && chan2_err_tot <= chan3_err_tot
		  && chan2_err_tot <= allchan_err_tot then begin
	chan2_err_tot, chan2_st, (Chan2 chan2_lev)
      end else if chan3_err_tot <= chan1_err_tot
                  && chan3_err_tot <= chan2_err_tot
		  && chan3_err_tot <= allchan_err_tot then begin
	chan3_err_tot, chan3_st, (Chan3 chan3_lev)
      end else begin
	allchan_err_tot, allchan_st, (Achan allchan_lev)
      end
    end
  and memoized_search st n lah =
    try
      Hashtbl.find memos (st, n, lah)
    with Not_found ->
      let (_, _, cmd) as res = search st n lah in
      begin match cmd with
        Nocmd -> ()
      | _ -> Hashtbl.add memos (st, n, lah) res
      end;
      res in
  let curstate = ref state in
  for i = 0 to (String.length outbuf) - 1 do
    let err, newstate, ccmd = memoized_search !curstate i lookahead in
    begin match ccmd with
      Chan1 n ->
	Printf.printf "%d: change channel 1 to %d: error (lookahead) is %f\n"
          i n err
    | Chan2 n ->
	Printf.printf "%d: change channel 2 to %d: error (lookahead) is %f\n"
          i n err
    | Chan3 n ->
	Printf.printf "%d: change channel 3 to %d: error (lookahead) is %f\n"
          i n err
    | Achan n ->
	Printf.printf "%d: change all channels to %d: error (lookahead) is %f\n"
          i n err
    | Nocmd -> ()
    end;
    let samp = int_of_float (255. *. (level newstate)) in
    outbuf.[i] <- Char.chr samp;
    curstate := newstate
  done;
  !curstate

let convert_file infile outfile quant =
  let inh = open_in_bin infile
  and outh = open_out_bin outfile in
  let inlen = in_channel_length inh
  and inbuf = String.create 8192
  and outbuf = String.make 4096 '\000' in
  let rec do_conversion bytes_remaining prev_state =
    if bytes_remaining > 0 then begin
      let bytes_to_do =
	if bytes_remaining < 8192 then bytes_remaining else 8192 in
      really_input inh inbuf 0 bytes_to_do;
      let next_state = sconv inbuf outbuf prev_state in
      output outh outbuf 0 4096;
      do_conversion (bytes_remaining - bytes_to_do) next_state
    end in
  ignore (do_conversion inlen { chan1 = 0; chan2 = 0; chan3 = 0})

let write_volumes voltab =
  let outbuf = String.make 768 '\000' in
  for chan = 1 to 3 do
    for vol = 0 to 255 do
      let chanbits =
        match chan, voltab.(vol) with
          1, (a, _, _) -> 0b11010000 lor (vol / 16)
	| 2, (_, b, _) -> 0b10110000 lor (vol / 16)
	| 3, (_, _, c) -> 0b10010000 lor (vol / 16)
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
  (* write_volumes voltab *)
