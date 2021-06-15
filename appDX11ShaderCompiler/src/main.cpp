#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <cstring>
#include <cstdlib>

#pragma comment(lib,"d3dcompiler.lib")

static void iDump(FILE *fp, const unsigned char *ptr, unsigned int len)
{
    for( int i = 0; i < len-1; i++ )
    {
        fprintf( fp, "%3d,", ptr[i] );
        if( ((i+1)&31)==0 ) fprintf(fp,"\n");
    }
    fprintf( fp, "%3d", ptr[len-1] );
}

struct Defines
{
    static constexpr int kMaxDefines = 16;
    int mNum;
    struct
    {
        char mName[64];
        int  mFirst;
        int  mLast;
        char mValue[4];
    }mDefine[kMaxDefines];
};

// fileIn fileOut main vs_5_0 -DSTEREOMODE=0..2 -DDRAWIN=0..1 -DWIGGLE=0..1 -DBRUSHTYPE=0..4 -DCOLOR_COMPRESSED=0..2
int main(int numArgs, const char** args)
{
    if (numArgs < 4 )
    {
        printf( "Error: too little arguments. Use:\n");
        printf( "DX11ShaderCompiler.exe fileIn fileOut main vs_5_0 -DSTEREOMODE=0..2 -DDRAWIN=0..1 -DWIGGLE=0..1 -DBRUSHTYPE=0..4 -DCOLOR_COMPRESSED=0..2\n");
        return 1;
    }

    // === extract shader name =========================

    char shaderName[128];

              const char *srcF = strrchr(args[1],'/');
    if( srcF == nullptr ) srcF = strrchr(args[1],'\\');
    if( srcF == nullptr ) srcF = args[1]; else srcF++;

    for( int i=0; i<srcF[i]!=0; i++ )
    {
        shaderName[i] = srcF[i];
        if( shaderName[i]=='.' )
        {
            shaderName[i] = 0;
            break;
        }
    }

    // === parse arguments and search for -D permutations =========================

    Defines defines;
    defines.mNum = 0;

    for (int i = 4; i<numArgs; i++)
    {
        if (args[i][0] == '-' && args[i][1] == 'D')
        {
            const char *src = args[i];
            int posFi = 0;
            int posLa = 0;
            char *dst = defines.mDefine[defines.mNum].mName;
            for( int i=2; src[i] != 0; i++ )
            {
                char c = src[i];
                if( c=='=') { posFi = i+1; c=0; }
                if( c=='.') { posLa = i+1; c=0; }
                *dst++ = c;
            }
            defines.mDefine[defines.mNum].mFirst = atoi(args[i]+posFi);
            defines.mDefine[defines.mNum].mLast = atoi(args[i]+posLa);
            defines.mNum++;
            if( defines.mNum>defines.kMaxDefines) break;
        }
    }

    // === open file =========================

    const char *strEntryPoint = args[3];
    const char *strProfile = args[4];
    wchar_t wstrFileInput[1024];
    if( mbstowcs(wstrFileInput, args[1], 1024)==-1 )
    {
        printf( "Error: invalid input file name\n");
        return 3;
    }

    FILE *fo = fopen( args[2], "wt");
    if (!fo)
    {
        printf( "Error: couldn't open file \"%s\".\n", args[2]);
        return 4;
    }

    fprintf( fo, "// Generated by DX11ShaderCompiler, part of the IMM SDK\n\n" );

    // === compile permutations =========================

    int numPermutations = 1;
    for( int i=0; i<defines.mNum; i++ )
    {
        numPermutations *= 1 + defines.mDefine[i].mLast - defines.mDefine[i].mFirst;
    }

    fprintf( fo, "// %d permutations\n\n", numPermutations );

    int  *shaderLengths = (int*)malloc(numPermutations*sizeof(int));
    if (!shaderLengths)
    {
        printf( "Error: internal error");
        fclose(fo);
        return 5;
    }

    char header[18];
    for( int i = 0; i < numPermutations; i++ )
    {
        D3D_SHADER_MACRO strDefines[Defines::kMaxDefines+1];

        int d = i;
        for( int j=0; j<defines.mNum; j++ )
        {
            const int v = d % (1 + defines.mDefine[j].mLast - defines.mDefine[j].mFirst);
            sprintf( defines.mDefine[j].mValue, "%d", v );

            header[j] ='0' + v; // note, this only works for a max of 10

            strDefines[j].Name = defines.mDefine[j].mName;
            strDefines[j].Definition = defines.mDefine[j].mValue;

            d /= (1 + defines.mDefine[j].mLast - defines.mDefine[j].mFirst);
        }
        strDefines[defines.mNum].Name = NULL;
        strDefines[defines.mNum].Definition = NULL;
        header[defines.mNum] = 0;

        ID3DBlob* shaderBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;
        HRESULT hr = D3DCompileFromFile( wstrFileInput, strDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                         strEntryPoint, strProfile, D3DCOMPILE_ENABLE_STRICTNESS, 0, &shaderBlob, &errorBlob );
        if ( FAILED(hr) )
        {
            printf("Error: failed compiling shader; error: %s\n", (char*)errorBlob->GetBufferPointer() );
            errorBlob->Release();
            return false;
        }


        const unsigned char *ptr = static_cast<unsigned char*>(shaderBlob->GetBufferPointer());
        const unsigned int len = static_cast<unsigned int>(shaderBlob->GetBufferSize());

        fprintf( fo, "static const unsigned char %s%s[%d] = {\n", shaderName, header, len );
        iDump( fo, ptr, len );
        fprintf( fo, "};\n\n" );

        shaderLengths[i] = len;

        shaderBlob->Release();
    }

    //=== write shader array ========================================================

    fprintf( fo, "static const unsigned char *%s_code[%d] = {", shaderName, numPermutations );
    for( int i = 0; i < numPermutations; i++ )
    {
        int d = i;
        for( int j=0; j<defines.mNum; j++ )
        {
            const int v = d % (1 + defines.mDefine[j].mLast - defines.mDefine[j].mFirst);
            header[j] ='0' + v; // note, this only works for a max of 10
            d /= (1 + defines.mDefine[j].mLast - defines.mDefine[j].mFirst);
        }
        fprintf( fo, "%s%s", shaderName, header );
        if( i!=(numPermutations-1)) fprintf( fo, ", ");
    }

    fprintf( fo, "};\n\n" );

    //=== write shader lengths ========================================================


    fprintf( fo, "static const int %s_size[%d] = {", shaderName, numPermutations );
    for( int i = 0; i < numPermutations; i++ )
    {
        fprintf( fo, "%d", shaderLengths[i]);
        if( i!=(numPermutations-1)) fprintf( fo, ", ");
    }
    fprintf( fo, "};\n\n" );
    free(shaderLengths);
    fclose(fo);

    printf( "%s ", shaderName);
    return 0;
}
