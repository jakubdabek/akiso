
primes:     file format elf32-i386


Disassembly of section .init:

080482ac <_init>:
 80482ac:	53                   	push   ebx
 80482ad:	83 ec 08             	sub    esp,0x8
 80482b0:	e8 8b 00 00 00       	call   8048340 <__x86.get_pc_thunk.bx>
 80482b5:	81 c3 4b 1d 00 00    	add    ebx,0x1d4b
 80482bb:	8b 83 fc ff ff ff    	mov    eax,DWORD PTR [ebx-0x4]
 80482c1:	85 c0                	test   eax,eax
 80482c3:	74 05                	je     80482ca <_init+0x1e>
 80482c5:	e8 36 00 00 00       	call   8048300 <__libc_start_main@plt+0x10>
 80482ca:	83 c4 08             	add    esp,0x8
 80482cd:	5b                   	pop    ebx
 80482ce:	c3                   	ret    

Disassembly of section .plt:

080482d0 <printf@plt-0x10>:
 80482d0:	ff 35 04 a0 04 08    	push   DWORD PTR ds:0x804a004
 80482d6:	ff 25 08 a0 04 08    	jmp    DWORD PTR ds:0x804a008
 80482dc:	00 00                	add    BYTE PTR [eax],al
	...

080482e0 <printf@plt>:
 80482e0:	ff 25 0c a0 04 08    	jmp    DWORD PTR ds:0x804a00c
 80482e6:	68 00 00 00 00       	push   0x0
 80482eb:	e9 e0 ff ff ff       	jmp    80482d0 <_init+0x24>

080482f0 <__libc_start_main@plt>:
 80482f0:	ff 25 10 a0 04 08    	jmp    DWORD PTR ds:0x804a010
 80482f6:	68 08 00 00 00       	push   0x8
 80482fb:	e9 d0 ff ff ff       	jmp    80482d0 <_init+0x24>

Disassembly of section .plt.got:

08048300 <.plt.got>:
 8048300:	ff 25 fc 9f 04 08    	jmp    DWORD PTR ds:0x8049ffc
 8048306:	66 90                	xchg   ax,ax

Disassembly of section .text:

08048310 <_start>:
 8048310:	31 ed                	xor    ebp,ebp
 8048312:	5e                   	pop    esi
 8048313:	89 e1                	mov    ecx,esp
 8048315:	83 e4 f0             	and    esp,0xfffffff0
 8048318:	50                   	push   eax
 8048319:	54                   	push   esp
 804831a:	52                   	push   edx
 804831b:	68 00 85 04 08       	push   0x8048500
 8048320:	68 a0 84 04 08       	push   0x80484a0
 8048325:	51                   	push   ecx
 8048326:	56                   	push   esi
 8048327:	68 45 84 04 08       	push   0x8048445
 804832c:	e8 bf ff ff ff       	call   80482f0 <__libc_start_main@plt>
 8048331:	f4                   	hlt    
 8048332:	66 90                	xchg   ax,ax
 8048334:	66 90                	xchg   ax,ax
 8048336:	66 90                	xchg   ax,ax
 8048338:	66 90                	xchg   ax,ax
 804833a:	66 90                	xchg   ax,ax
 804833c:	66 90                	xchg   ax,ax
 804833e:	66 90                	xchg   ax,ax

08048340 <__x86.get_pc_thunk.bx>:
 8048340:	8b 1c 24             	mov    ebx,DWORD PTR [esp]
 8048343:	c3                   	ret    
 8048344:	66 90                	xchg   ax,ax
 8048346:	66 90                	xchg   ax,ax
 8048348:	66 90                	xchg   ax,ax
 804834a:	66 90                	xchg   ax,ax
 804834c:	66 90                	xchg   ax,ax
 804834e:	66 90                	xchg   ax,ax

08048350 <deregister_tm_clones>:
 8048350:	b8 1c a0 04 08       	mov    eax,0x804a01c
 8048355:	3d 1c a0 04 08       	cmp    eax,0x804a01c
 804835a:	74 24                	je     8048380 <deregister_tm_clones+0x30>
 804835c:	b8 00 00 00 00       	mov    eax,0x0
 8048361:	85 c0                	test   eax,eax
 8048363:	74 1b                	je     8048380 <deregister_tm_clones+0x30>
 8048365:	55                   	push   ebp
 8048366:	89 e5                	mov    ebp,esp
 8048368:	83 ec 14             	sub    esp,0x14
 804836b:	68 1c a0 04 08       	push   0x804a01c
 8048370:	ff d0                	call   eax
 8048372:	83 c4 10             	add    esp,0x10
 8048375:	c9                   	leave  
 8048376:	c3                   	ret    
 8048377:	89 f6                	mov    esi,esi
 8048379:	8d bc 27 00 00 00 00 	lea    edi,[edi+eiz*1+0x0]
 8048380:	c3                   	ret    
 8048381:	eb 0d                	jmp    8048390 <register_tm_clones>
 8048383:	90                   	nop
 8048384:	90                   	nop
 8048385:	90                   	nop
 8048386:	90                   	nop
 8048387:	90                   	nop
 8048388:	90                   	nop
 8048389:	90                   	nop
 804838a:	90                   	nop
 804838b:	90                   	nop
 804838c:	90                   	nop
 804838d:	90                   	nop
 804838e:	90                   	nop
 804838f:	90                   	nop

08048390 <register_tm_clones>:
 8048390:	b8 1c a0 04 08       	mov    eax,0x804a01c
 8048395:	2d 1c a0 04 08       	sub    eax,0x804a01c
 804839a:	c1 f8 02             	sar    eax,0x2
 804839d:	89 c2                	mov    edx,eax
 804839f:	c1 ea 1f             	shr    edx,0x1f
 80483a2:	01 d0                	add    eax,edx
 80483a4:	d1 f8                	sar    eax,1
 80483a6:	74 20                	je     80483c8 <register_tm_clones+0x38>
 80483a8:	ba 00 00 00 00       	mov    edx,0x0
 80483ad:	85 d2                	test   edx,edx
 80483af:	74 17                	je     80483c8 <register_tm_clones+0x38>
 80483b1:	55                   	push   ebp
 80483b2:	89 e5                	mov    ebp,esp
 80483b4:	83 ec 10             	sub    esp,0x10
 80483b7:	50                   	push   eax
 80483b8:	68 1c a0 04 08       	push   0x804a01c
 80483bd:	ff d2                	call   edx
 80483bf:	83 c4 10             	add    esp,0x10
 80483c2:	c9                   	leave  
 80483c3:	c3                   	ret    
 80483c4:	8d 74 26 00          	lea    esi,[esi+eiz*1+0x0]
 80483c8:	c3                   	ret    
 80483c9:	8d b4 26 00 00 00 00 	lea    esi,[esi+eiz*1+0x0]

080483d0 <__do_global_dtors_aux>:
 80483d0:	80 3d 1c a0 04 08 00 	cmp    BYTE PTR ds:0x804a01c,0x0
 80483d7:	75 17                	jne    80483f0 <__do_global_dtors_aux+0x20>
 80483d9:	55                   	push   ebp
 80483da:	89 e5                	mov    ebp,esp
 80483dc:	83 ec 08             	sub    esp,0x8
 80483df:	e8 6c ff ff ff       	call   8048350 <deregister_tm_clones>
 80483e4:	c6 05 1c a0 04 08 01 	mov    BYTE PTR ds:0x804a01c,0x1
 80483eb:	c9                   	leave  
 80483ec:	c3                   	ret    
 80483ed:	8d 76 00             	lea    esi,[esi+0x0]
 80483f0:	c3                   	ret    
 80483f1:	eb 0d                	jmp    8048400 <frame_dummy>
 80483f3:	90                   	nop
 80483f4:	90                   	nop
 80483f5:	90                   	nop
 80483f6:	90                   	nop
 80483f7:	90                   	nop
 80483f8:	90                   	nop
 80483f9:	90                   	nop
 80483fa:	90                   	nop
 80483fb:	90                   	nop
 80483fc:	90                   	nop
 80483fd:	90                   	nop
 80483fe:	90                   	nop
 80483ff:	90                   	nop

08048400 <frame_dummy>:
 8048400:	eb 8e                	jmp    8048390 <register_tm_clones>

08048402 <is_prime>:
 8048402:	55                   	push   ebp
 8048403:	89 e5                	mov    ebp,esp
 8048405:	83 ec 10             	sub    esp,0x10
 8048408:	83 7d 08 01          	cmp    DWORD PTR [ebp+0x8],0x1
 804840c:	7f 07                	jg     8048415 <is_prime+0x13>
 804840e:	b8 00 00 00 00       	mov    eax,0x0
 8048413:	eb 2e                	jmp    8048443 <is_prime+0x41>
 8048415:	c7 45 fc 02 00 00 00 	mov    DWORD PTR [ebp-0x4],0x2
 804841c:	eb 18                	jmp    8048436 <is_prime+0x34>
 804841e:	8b 45 08             	mov    eax,DWORD PTR [ebp+0x8]
 8048421:	99                   	cdq    
 8048422:	f7 7d fc             	idiv   DWORD PTR [ebp-0x4]
 8048425:	89 d0                	mov    eax,edx
 8048427:	85 c0                	test   eax,eax
 8048429:	75 07                	jne    8048432 <is_prime+0x30>
 804842b:	b8 00 00 00 00       	mov    eax,0x0
 8048430:	eb 11                	jmp    8048443 <is_prime+0x41>
 8048432:	83 45 fc 01          	add    DWORD PTR [ebp-0x4],0x1
 8048436:	8b 45 fc             	mov    eax,DWORD PTR [ebp-0x4]
 8048439:	3b 45 08             	cmp    eax,DWORD PTR [ebp+0x8]
 804843c:	7c e0                	jl     804841e <is_prime+0x1c>
 804843e:	b8 01 00 00 00       	mov    eax,0x1
 8048443:	c9                   	leave  
 8048444:	c3                   	ret    

08048445 <main>:
 8048445:	8d 4c 24 04          	lea    ecx,[esp+0x4]
 8048449:	83 e4 f0             	and    esp,0xfffffff0
 804844c:	ff 71 fc             	push   DWORD PTR [ecx-0x4]
 804844f:	55                   	push   ebp
 8048450:	89 e5                	mov    ebp,esp
 8048452:	51                   	push   ecx
 8048453:	83 ec 14             	sub    esp,0x14
 8048456:	c7 45 f4 01 00 00 00 	mov    DWORD PTR [ebp-0xc],0x1
 804845d:	eb 26                	jmp    8048485 <main+0x40>
 804845f:	ff 75 f4             	push   DWORD PTR [ebp-0xc]
 8048462:	e8 9b ff ff ff       	call   8048402 <is_prime>
 8048467:	83 c4 04             	add    esp,0x4
 804846a:	84 c0                	test   al,al
 804846c:	74 13                	je     8048481 <main+0x3c>
 804846e:	83 ec 08             	sub    esp,0x8
 8048471:	ff 75 f4             	push   DWORD PTR [ebp-0xc]
 8048474:	68 20 85 04 08       	push   0x8048520
 8048479:	e8 62 fe ff ff       	call   80482e0 <printf@plt>
 804847e:	83 c4 10             	add    esp,0x10
 8048481:	83 45 f4 01          	add    DWORD PTR [ebp-0xc],0x1
 8048485:	81 7d f4 0f 27 00 00 	cmp    DWORD PTR [ebp-0xc],0x270f
 804848c:	7e d1                	jle    804845f <main+0x1a>
 804848e:	b8 00 00 00 00       	mov    eax,0x0
 8048493:	8b 4d fc             	mov    ecx,DWORD PTR [ebp-0x4]
 8048496:	c9                   	leave  
 8048497:	8d 61 fc             	lea    esp,[ecx-0x4]
 804849a:	c3                   	ret    
 804849b:	66 90                	xchg   ax,ax
 804849d:	66 90                	xchg   ax,ax
 804849f:	90                   	nop

080484a0 <__libc_csu_init>:
 80484a0:	55                   	push   ebp
 80484a1:	57                   	push   edi
 80484a2:	56                   	push   esi
 80484a3:	53                   	push   ebx
 80484a4:	e8 97 fe ff ff       	call   8048340 <__x86.get_pc_thunk.bx>
 80484a9:	81 c3 57 1b 00 00    	add    ebx,0x1b57
 80484af:	83 ec 0c             	sub    esp,0xc
 80484b2:	8b 6c 24 20          	mov    ebp,DWORD PTR [esp+0x20]
 80484b6:	8d b3 10 ff ff ff    	lea    esi,[ebx-0xf0]
 80484bc:	e8 eb fd ff ff       	call   80482ac <_init>
 80484c1:	8d 83 0c ff ff ff    	lea    eax,[ebx-0xf4]
 80484c7:	29 c6                	sub    esi,eax
 80484c9:	c1 fe 02             	sar    esi,0x2
 80484cc:	85 f6                	test   esi,esi
 80484ce:	74 25                	je     80484f5 <__libc_csu_init+0x55>
 80484d0:	31 ff                	xor    edi,edi
 80484d2:	8d b6 00 00 00 00    	lea    esi,[esi+0x0]
 80484d8:	83 ec 04             	sub    esp,0x4
 80484db:	ff 74 24 2c          	push   DWORD PTR [esp+0x2c]
 80484df:	ff 74 24 2c          	push   DWORD PTR [esp+0x2c]
 80484e3:	55                   	push   ebp
 80484e4:	ff 94 bb 0c ff ff ff 	call   DWORD PTR [ebx+edi*4-0xf4]
 80484eb:	83 c7 01             	add    edi,0x1
 80484ee:	83 c4 10             	add    esp,0x10
 80484f1:	39 f7                	cmp    edi,esi
 80484f3:	75 e3                	jne    80484d8 <__libc_csu_init+0x38>
 80484f5:	83 c4 0c             	add    esp,0xc
 80484f8:	5b                   	pop    ebx
 80484f9:	5e                   	pop    esi
 80484fa:	5f                   	pop    edi
 80484fb:	5d                   	pop    ebp
 80484fc:	c3                   	ret    
 80484fd:	8d 76 00             	lea    esi,[esi+0x0]

08048500 <__libc_csu_fini>:
 8048500:	f3 c3                	repz ret 

Disassembly of section .fini:

08048504 <_fini>:
 8048504:	53                   	push   ebx
 8048505:	83 ec 08             	sub    esp,0x8
 8048508:	e8 33 fe ff ff       	call   8048340 <__x86.get_pc_thunk.bx>
 804850d:	81 c3 f3 1a 00 00    	add    ebx,0x1af3
 8048513:	83 c4 08             	add    esp,0x8
 8048516:	5b                   	pop    ebx
 8048517:	c3                   	ret    
